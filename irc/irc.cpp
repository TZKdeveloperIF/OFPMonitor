

#include <winsock.h>
#include <list>
#include <iostream.h>
#include "Unit1.h" 
#include "irc.h"
#include <vector.h>
#include <set.h>
#include <time.h>
#include <sstream>

                
//#define DEBUG_MSG
#ifdef DEBUG_MSG
#define INPUTOUT 1
#else             
#define INPUTOUT 0
#endif

extern int tcpsocket(void) ;
extern unsigned long dnsdb(char *host);

static DWORD WINAPI irc_ThreadProc( LPVOID lpThreadParameter );
static struct irc_thread__parm * p ;
static void getplayername( ); 
static void start_conversation( int sd, char * name ); 
static void sendMessage(const char * xmsg);
static string channelname_operationflashpoint1 ("operationflashpoint1");

static string after(string& in, string& needle);
static string before(string& in, string& needle);
static int starts(string& in, string& needle);
extern unsigned long resolv(char *host) ;  
static string currrentTimeString();    
static string plrname_localtoirc(  char * name  );
static string name_irctolocal(string& n);



struct irc_thread__parm {
    TForm1 * tform1 ;
    vector<string> messages;
    vector<string> userz;
    set<string> userzSorted;
    string hoscht;

    int updatePlayers;
    int sd;
    int loggedIn;
    void consume(char* c, int i);
    int sentVersion;
    public:
    irc_thread__parm():sentVersion(0),updatePlayers(0){
    }
    int sendString(string&);
};
static char playerName[1024];

void chat_client_disconnect() {
    if (p && p->sd) {
        closesocket(p->sd);
        p->sd = 0;
        p->hoscht.clear();
    }
}


void chat_client_connect(  void * tf ) {
   TForm1 * tform1  = ( TForm1    *  )  tf;
   getplayername();
   if (strlen(playerName) < 1){
        return;
   }
    if (!p) {
        p = new irc_thread__parm();
    }
    CreateThread(0 , 0 , irc_ThreadProc , p , 0 , 0);
}

static string name_irctolocal(string& n){
        string k("ofpmon_");
        int p = n.find(k,0);
        if (p >= 0){
                return string( n , p+k.size() );
        }
        return n;
}
   

string plrname_localtoirc(  char * name  ){
    string n ( name );
    int i;
    for(i = 0; i < n.size(); i++){
        char c = n.at(i);
        int isSmall = c >= 'a' && c <= 'z';
        int isBig = c >= 'A' && c <= 'Z';
        int isNum = c >= '0' && c <= '9';

        if (  !isSmall &&
              !isBig &&
              !(i > 1 && isNum)
                ){
            n[i]='_';
        }
    }
    return "ofpmon_" + n;
}

void appendText( TForm1 * tform1, string& msg ){
                TRichEdit * tr =  tform1->RichEditChatContent;
                AnsiString& as = tr->Text;//-> = false;
                as += AnsiString(msg.c_str());
                as += "\r\n";
                tr->Text = as;
}


static string currrentTimeString(){
  time_t rawtime;
  struct tm * timeinfo;
  char buffer [80];

  time ( &rawtime );
  timeinfo = localtime ( &rawtime );

  strftime (buffer,80,"%H:%M ",timeinfo);
 return buffer; 

     }
void chat_client_timercallback(  void * t ){
    TForm1 * tform1  = (TForm1 *) t;

    if (p && p->messages.size() > 0) {
         vector<string> m  (p->messages);
         p->messages.clear();

        string privMsgNeedle = "PRIVMSG #" + channelname_operationflashpoint1 +" :";
         for( int i = 0; i < m.size(); i++) {

                string& omsg = m.at(i);

                
                if (INPUTOUT) {
                     appendText(tform1 , omsg);
                }

                 string cmsg = omsg;
                 string playername;
                 int fnd = 0;
                 if ((fnd = cmsg.find(privMsgNeedle,0)) >= 0) {
                     cmsg = string( cmsg, fnd + privMsgNeedle.size()  );
                     int emp = omsg.find("!",1);
                     playername = string( omsg, 1, emp - 1 );
                     playername = name_irctolocal(playername);
                     cmsg = currrentTimeString() + " - " + playername + ": " + cmsg;

//                     ctime_r ();
//asctime_r( 0, 0);
//ctime_r( 0, 0);

                     appendText(tform1 , cmsg);
                 }


         }
    }

    if (0 && p && p->userz.size() > 0){
         vector<string> m  (p->userz);
         p->userz.clear();
         for( int i = 0; i < m.size(); i++) {
                TStringGrid * tssg = tform1->StringGrid3;
                int rc =  tssg->RowCount;
                tssg->RowCount = rc + 1;
                string& stre = m.at(i);
                string convertedPlayerName = name_irctolocal( stre );
                tssg->Cells[0][rc] = convertedPlayerName.c_str();
         }
    }

    if (p->updatePlayers){
       p->updatePlayers = 0;
       set<string> userzSortedCopy = set<string>(p->userzSorted);
       TStringGrid * tssg = tform1->StringGrid3;
       tssg->RowCount = userzSortedCopy.size();

       // convert to vector
       vector<string> ulist( userzSortedCopy.begin() ,userzSortedCopy.end()  );
       for(int i = 0; i < ulist.size(); i++) {
             tssg->Cells[0][i] = ulist[i].c_str();
       }

    }
}



DWORD WINAPI irc_ThreadProc (LPVOID lpdwThreadParam__ ) {
    int sd = tcpsocket();
    struct sockaddr_in addr;
    struct irc_thread__parm * p_parm = (struct irc_thread__parm *) lpdwThreadParam__;
    TForm1 * tform1  = p_parm->tform1;
    p_parm->sd = sd;

    memset( &addr , 0 , sizeof(addr));
    // irc.freenode.net = 140.211.167.98
    // int ip = inet_addr("140.211.167.98");
    //int ip = dnsdb("irc.freenode.net");
int ip =    resolv("irc.freenode.net");
    addr.sin_addr.s_addr = ip;
    addr.sin_port        = htons(6666);
    addr.sin_family      = AF_INET;

    int connectRes = connect(sd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
      start_conversation(sd, playerName);
      if( connectRes >= 0) {
        char buff [1<<10];
        int r;
            while(p_parm->sd && (r = recv(sd, buff, sizeof(buff) , 0)) > 0){
                    TRichEdit * tr =  tform1->RichEditChatContent;
                    p_parm->consume( buff,r );
            } 
      }
}



int irc_thread__parm::sendString(string& s) {
     return   send(sd, s.c_str(), s.length(), 0);
}


void start_conversation( int sd, char * name ) {

      string ircName =   plrname_localtoirc(name);

    stringstream ss;

    ss << "CAP LS\n"
      "NICK " << ircName << "\n"
        << "USER " << ircName << " 0 * :" << ircName << "\n"
        << "CAP REQ :multi-prefix\n"
        <<  "CAP END\n"
        << "USERHOST "<<  ircName <<  "\n"
        << "JOIN #" << channelname_operationflashpoint1 << "\n"
        << "MODE #" << channelname_operationflashpoint1 << "\n";

        string msg =    ss.str();
     int s = send(sd, msg.c_str(), msg.length(), 0);  

     return;
}


void  getplayername( ){
    //http://help.github.com/fork-a-repo/
    //"HKEY_CURRENT_USER\Software\Codemasters\Operation Flashpoint"
    char lszValue[100];
    LONG lRet, lEnumRet;
    HKEY hKey = 0;
    DWORD dwLength=100;
    int i=0;

    //char* target=(char*) &lszValue;
    //target=playerName;

    lRet = RegOpenKeyEx (HKEY_CURRENT_USER, "Software\\Codemasters\\Operation Flashpoint", 0L, KEY_READ , &hKey);
    if(lRet == ERROR_SUCCESS)
    {
        DWORD dwType=REG_SZ;
        DWORD dwSize=255;
        lEnumRet = RegQueryValueEx(hKey, "Player Name", NULL, &dwType,(LPBYTE)playerName, &dwSize);
       }
     if (hKey) {
        RegCloseKey(hKey);
    }
}

static vector<string> explode(string s){
        vector<string> r;

        if(s.size()>0){
        int p = 0;
        int t = 0;
                while( p < s.size() &&
                 (t = s.find( "\r\n" ,p ) ) > p){
                        string line (s, p, t-p);
                        r.push_back( line );
                        p = t + 2;
                }
            if (p < s.size() - 1){
                   r.push_back( string(s, p, s.size() - p) );
            }

        }
        return r;
}

void irc_thread__parm::consume(char* c2, int i2) {
        vector<string> msgs =  explode( string(c2,i2) );
        int it = 0;
        for(;it < msgs.size(); it++ ){
            string& s = msgs.at(it);

            if (hoscht.size() == 0) {
              int p = s.find(" NOTICE" , 0);
              if (p > 0) {
                  hoscht = string( s , 0, p );
              }
            }


            //string playerzNeedle( hoscht + " 353 " );
            string body = after(s , " ");
            if (starts(body , "353 ")) {
            string ps2 = after( body , ":" );
                    while (ps2.size() > 0 ) {
                         int isLastPlayer = ps2.find( " " , 0 ) == -1;
                         string player;
                         if (isLastPlayer) {
                            player = ps2;
                         } else {
                            player = before(ps2, " ");
                         }
                         player = name_irctolocal(player);
                         userz.push_back(player);
                         userzSorted.insert(player);
                         updatePlayers = 1;
                         if (isLastPlayer) {
                            break;
                         }
                         ps2 = after(ps2, " ");
                    }
            }
            
            int pingFind =  s.find("PING " + hoscht, 0) ;
            int joinFind = s.find( " JOIN " , 0) ;
            int partFind = s.find( " PART " , 0) ;
            int endNameListFind = s.find("End of /NAMES list.",0);
            
            if ( pingFind == 0 ) {
              // sending pong
              string pong ("PONG " + hoscht + "\r\n");
              send(sd, pong.c_str(), pong.length(), 0);
            } else if (!sentVersion && endNameListFind >= 0) {
                sentVersion = 1;
                sendMessage( "Logged in with OFPMonitor version "  OFPMONITOR_VERSIO_REPORT);
            } else if ( joinFind > 0 ) {
              string name = after(s,":");
              name = before(name, "!");
              name=name_irctolocal(name);
              userzSorted.insert(name);
              updatePlayers = 1;
            } else if ( partFind > 0 ) {
              string name = after(s,":");
              name = before(name, "!"); 
              name=name_irctolocal(name);
              userzSorted.erase(name);
              updatePlayers = 1;
            }
            messages.push_back( s );
    }
}



void sendMessage(const char * xmsg){

        string msg( xmsg );
        msg = "PRIVMSG #" + channelname_operationflashpoint1 + " :" + msg + "\r\n";
        send(p->sd, msg.c_str(), msg.length(), 0);
}



void chat_client_pressedReturnKey(  void * t ) {
    TForm1 * tform1  = (TForm1 *) t;
    TEdit* te = tform1->Edit5;
    AnsiString as = te->Text;
    te->Text = "";

    if (p && p->sd) {
         sendMessage(as.c_str());
         appendText( tform1 ,  currrentTimeString( ) +  " - <me>: " + as.c_str()  );
    }
}


static string after(string& in, string& needle){
  int i = in.find(needle, 0);
  if (i >= 0){
    return string(in, i+needle.length());
  }
  return "";
}


static string before(string& in, string& needle){   
  int i = in.find(needle, 0);
  if (i > 0){
    return string(in, 0, i);
  }
  return "";
}

static int starts(string& in, string& needle) {
return in.find(needle,0) == 0;
}
