#ifndef CONSOLE_OUTPUT_H
#define CONSOLE_OUTPUT_H

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <ipc/ExtsanData.h>

class ConsoleOutput{

    std::shared_ptr<spdlog::logger> logger;

public:
    ConsoleOutput(){
        logger = spdlog::stdout_logger_st("console");
	    logger->set_level(spdlog::level::debug);
    }

    void makeOutput(std::shared_ptr<ipc::event::BufferEntry> buf){
        switch(buf->type){
            case ipc::event::Type::FUNCENTER:
                logger->debug("funcenter, call {}", buf->payload.funcenter.pc);
                break;
            case ipc::event::Type::FUNCEXIT:
                logger->debug("funcexit");
                break;
            case ipc::event::Type::MEMREAD:
                logger->debug("memread, pc {}", buf->payload.memaccess.pc);
                break;
            case ipc::event::Type::MEMWRITE:
                logger->debug("memwrite, pc {}", buf->payload.memaccess.pc);
                break;
            case ipc::event::Type::ACQUIRE:
                logger->debug("aquire, addr {}", buf->payload.mutex.addr);
                break;
            case ipc::event::Type::RELEASE:
                logger->debug("release, addr {}", buf->payload.mutex.addr);
                break;
            case ipc::event::Type::HAPPENSBEFORE:
                logger->debug("happensbefore, id {}", buf->payload.happens.id);
                break;
            case ipc::event::Type::HAPPENSAFTER:
                logger->debug("happensafter, id {}", buf->payload.happens.id);
                break;
            case ipc::event::Type::ALLOCATION:
                logger->debug("alloc, addr {}", buf->payload.allocation.addr);
                break;
            case ipc::event::Type::FREE:
                logger->debug("dealloc, addr {}", buf->payload.allocation.addr);
                break;
            case ipc::event::Type::FORK:
                logger->debug("fork, threadid {}", buf->payload.forkjoin.child);
                break;
            case ipc::event::Type::JOIN:
                logger->debug("join, threadid {}", buf->payload.forkjoin.child);
                break;
            case ipc::event::Type::DETACH:
                logger->debug("detach, threadid {}", buf->payload.detachfinish.thread_id);
                break;
            case ipc::event::Type::FINISH:
                logger->debug("finish, threadid {}", buf->payload.detachfinish.thread_id);
                break;
        }
    }

};
#endif //CONSOLE_OUTPUT_H