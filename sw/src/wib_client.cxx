#include <zmq.hpp>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "wib.pb.h"

template <class R, class C>
void send_command(zmq::socket_t &socket, const C &msg, R &repl) {

    wib::Command command;
    command.mutable_cmd()->PackFrom(msg);
    
    std::string cmd_str;
    command.SerializeToString(&cmd_str);
    
    zmq::message_t request(cmd_str.size());
    memcpy((void*)request.data(), cmd_str.c_str(), cmd_str.size());
    socket.send(request);
    
    zmq::message_t reply;
    socket.recv(&reply,0);
    
    std::string reply_str(static_cast<char*>(reply.data()), reply.size());
    repl.ParseFromString(reply_str);
    
}

int run_command(zmq::socket_t &s, int argc, char **argv) {
    if (argc < 1) return 1;
    
    std::string cmd(argv[0]);
    if (cmd == "exit") { 
        return 255;
    } else if (cmd == "get_sensors") {
        wib::GetSensors req;
        wib::Sensors rep;
        send_command(s,req,rep);
    } else if (cmd == "update") {
        wib::Update req;
        wib::Empty rep;
        send_command(s,req,rep);
    } else if (cmd == "initialize") {
        wib::Initialize req;
        wib::Empty rep;
        send_command(s,req,rep);
    } else if (cmd == "reboot") {
        wib::Reboot req;
        wib::Empty rep;
        send_command(s,req,rep);
    } else if (cmd == "peek") {
        if (argc != 2) {
            fprintf(stderr,"Usage: peek addr\n");
            return 0;
        }
        wib::Peek req;
        req.set_addr((size_t)strtoull(argv[1],NULL,16));
        wib::RegValue rep;
        send_command(s,req,rep);
        printf("peek 0x%lX 0x%X\n",rep.addr(),rep.value());
    } else if (cmd == "poke") {
        if (argc != 3) {
            fprintf(stderr,"Usage: poke addr value\n");
            return 0;
        }
        wib::Poke req;
        req.set_addr((size_t)strtoull(argv[1],NULL,16));
        req.set_value((uint32_t)strtoull(argv[2],NULL,16));
        wib::RegValue rep;
        send_command(s,req,rep);
        printf("peek 0x%lX 0x%X\n",rep.addr(),rep.value());
    } else {
        fprintf(stderr,"Unrecognized Command: %s\n",argv[0]);
        return 0;
    }
    return 0;
}

int main(int argc, char **argv) {
    
    char *ip = (char*)"127.0.0.1";
    
    char opt;
    while ((opt = getopt(argc, argv, "w:")) != -1) {
       switch (opt) {
           case 'w':
               ip = optarg;
               break;
           default: /* '?' */
               fprintf(stderr, "Usage: %s [-w ip] [cmd] \n", argv[0]);
               return 1;
       }
    }
    
    
    zmq::context_t context(1);
    zmq::socket_t socket(context, ZMQ_REQ);
    
    char *addr;
    asprintf(&addr,"tcp://%s:1234",ip);
    socket.connect(addr);
    free(addr);
    
    if (optind < argc) {
        return run_command(socket,argc-optind,argv+optind);
    } else {
        char* buf;
        while ((buf = readline(">> ")) != nullptr) {
            if (strlen(buf) > 0) {
                add_history(buf);
            } else {
                free(buf);
                continue;
            }
            char *delim = " ";   
            int count = 1;
            char *ptr = buf;
            while((ptr = strchr(ptr, delim[0])) != NULL) {
                count++;
                ptr++;
            }
            if (count > 0) {
                char **cmd = new char*[count];
                cmd[0] = strtok(buf, delim);
                for (int i = 1; cmd[i-1] != NULL; i++) {
                    cmd[i] = strtok(NULL, delim);
                }
                int ret = run_command(socket,count,cmd);
                delete [] cmd;
                if (ret == 255) return 0;
                if (ret != 0) return ret;
            } else {
                return 0;
            }
            free(buf);
        }
    }
    
    return 0;
}