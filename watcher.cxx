#include <stdio.h>
#include <iostream>
#include <fstream>
//#include <thread>
//#include <mutex>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/inotify.h>
//#include <condition_variable>
#include <unistd.h>
#include <signal.h>
#include <sstream>
#include<fcntl.h> // library for fcntl function
using namespace std;

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )
//#define READ_CONF_TIME 300 //in seconds

#define TRACE(x)  { std::cout << x; }
	
//std::set<string> directories_set; // global?
//std::mutex m;
//std::condition_variable cv;

std::string g_directories_to_monitor;
int fd;
int wd;
//static int ANY_THREAD_IS_WORKING;
class CLogger
{
	static CLogger * obj;
	string file_name;
	pid_t pid ;
	static std::ofstream of;
	CLogger(){
		 pid = getpid();
		file_name_set();
		of.open((const char *)file_name.c_str(),ios::app| ios::out);
	};
	
	CLogger(const CLogger &){};
	public:
	const std::string currentDateTime() {
		time_t     now = time(0);
		struct tm  tstruct;
		char       buf[80];
		tstruct = *localtime(&now);
		strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);
		return buf;
	}
	const std::string currentDate() {
		time_t     now = time(0);
		struct tm  tstruct;
		char       buf[80];
		tstruct = *localtime(&now);
		strftime(buf, sizeof(buf), "%Y-%m-%d", &tstruct);
		return buf;
	}

	static CLogger* getinstance();
	void file_name_set()
	{
		file_name = currentDate();
		file_name = file_name + "_history.log";
		
	}
	void write(string str)
	{
		if(!(of.is_open()))
		{
			 pid = getpid();
			file_name_set();
			of.open((const char *)file_name.c_str(),std::ios::app| std::ios::out);
		}
		
		str=str+"\n";
		//m.lock();
		of << currentDateTime() << " ";
		of << pid <<" ";
		of << str.c_str();
		//m.unlock();
	}
		
	~CLogger()
	{
		if(obj)
			delete obj;
		if(of)
			of.close();
	}
};

CLogger * CLogger::obj;
ofstream CLogger::of;
CLogger * CLogger::getinstance()
	{
		if(obj==NULL)
			obj=new CLogger();
		return obj;
	}

void sig_handler(int sig){
 
       /* Step 5. Remove the watch descriptor and close the inotify instance*/
       inotify_rm_watch( fd, wd );
       close( fd );
       exit( 0 );
 
}

void read_config(std::string cfgfile)
{
	//TRACE("ENTRY:: INTO READ CONFIG");
	std::string line;
	std::ifstream cFile(cfgfile);
	if (cFile.is_open())
	{
			/*unique_lock<mutex> ul(m);
			cv.wait(ul, []()
			{
				return ANY_THREAD_IS_WORKING ? false : true;
			});
			m.lock();
			ANY_THREAD_IS_WORKING = 1;*/
			/*for (std::string line; std::getline(cfgfile, line);)
			{
				directories_set.insert(line);
			}  */
			std::getline(cFile,line);
			g_directories_to_monitor = line;
			//TRACE(g_directories_to_monitor);
			/*ANY_THREAD_IS_WORKING = 0;
			m.unlock();
			cv.notify_all();*/
			cFile.close();
			//std::this_thread::sleep_for(std::chrono::seconds(READ_CONF_TIME));
	}
	else
	{
			cout << "File is not present" << endl;
			exit(0);

	}
	
}

void monitor_directory()
{
    signal(SIGINT,sig_handler);
	CLogger * write_ins=CLogger::getinstance();
	std::stringstream ss;
       /* Step 1. Initialize inotify */
       fd = inotify_init();
 
 
       if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)  // error checking for fcntl
       exit(2);
 
       /* Step 2. Add Watch */
       wd = inotify_add_watch(fd,(const char*) g_directories_to_monitor.c_str(),IN_MODIFY | IN_CREATE | IN_DELETE);
 
       if(wd==-1){
               printf("Could not watch : %s\n",g_directories_to_monitor.c_str());
       }
       else{
		   ss <<"****Process Restarted.**** Monitoring directory ::" << g_directories_to_monitor.c_str();
			write_ins->write(ss.str());
			ss.str("");
              //printf("Watching : %s\n",g_directories_to_monitor.c_str());
       }
	   
 
       while(1){
 
              int i=0,length;
              char buffer[BUF_LEN];
 
              /* Step 3. Read buffer*/
              length = read(fd,buffer,BUF_LEN);
 
              /* Step 4. Process the events which has occurred */
             while(i<length){
 
                struct inotify_event *event = (struct inotify_event *) &buffer[i];
				TRACE("Event recieved::" << event->mask);
				
                  if(event->len){
                   if ( event->mask & IN_CREATE ) {
                   if ( event->mask & IN_ISDIR ) {
                     ss << "The directory " << event->name << " was created.";
					// printf( "The directory %s was created.\n", event->name );
                     }
                     else {
                       ss << "The file " << event->name << " was created.";
					  // printf( "The file %s was created.\n", event->name );
                    }
				   }
                    else if ( event->mask & IN_DELETE ) {
                    if ( event->mask & IN_ISDIR ) {
						ss << "The directory " << event->name << " was deleted.";
                      //printf( "The directory %s was deleted.\n", event->name );
                    }
                    else {
						ss << "The file " << event->name << " was deleted.";
                      //printf( "The file %s was deleted.\n", event->name );
                    }
                    }
                    else if ( event->mask & IN_MODIFY ) {
                    if ( event->mask & IN_ISDIR ) {
						ss << "The directory " << event->name << " was modified.";
                      //printf( "The directory %s was modified.\n", event->name );
                    }
                    else {
						ss << "The file " << event->name << " was modified.";
                     //printf( "The file %s was modified.\n", event->name );
                    }
                    }
					
                   }
				    //TRACE(ss.str());
					write_ins->write(ss.str());
					ss.str("");
                   i += EVENT_SIZE + event->len;
				   
          }  
    }
}


int main(int argc, char **argv)
{
	
	if (argc < 2)
	{
		cout << "ERROR ::: The executable should contain path of .ini file." <<endl;
		cout << "INI file shall contain only the path of the directory." <<endl;
		return 0;
	}
	//cout<<"calling threads";
	//std::thread READ_CONFIG_THREAD(read_config, argv[1]);
	//std::thread FILE_MONITOR_THREAD(monitor_directory);
	read_config(argv[1]);
	monitor_directory();
	//READ_CONFIG_THREAD.join();
	//FILE_MONITOR_THREAD.join(); 
	return 0;
}