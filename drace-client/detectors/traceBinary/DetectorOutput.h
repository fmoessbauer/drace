#ifndef DETECTOR_OUTPUT_H
#define DETECTOR_OUTPUT_H

#include <ipc/ExtsanData.h>

#include <detector/Detector.h>
#include <util/WindowsLibLoader.h>
#include <Windows.h>

class DetectorOutput{

    std::unordered_map<uint32_t, void*> tls; 

    void * allocate_memory(uint32_t tid){
        void * mem = new uint64_t;
        tls.insert({tid, mem});
        return mem;
    }

    void fork(uint32_t tid, uint32_t parent_tid){
        void * mem = allocate_memory(tid);
        _det->fork(parent_tid, tid, &mem);
    }

protected:
    util::WindowsLibLoader _libdetector;
    std::unique_ptr<Detector> _det;

public:
    DetectorOutput(){
        
        _libdetector.load("drace.detector.fasttrack.standalone.dll");
        decltype(CreateDetector)* create_detector = _libdetector["CreateDetector"];
        _det =  std::unique_ptr<Detector>(create_detector());

        const char * _argv = "";
		_det->init(1, &_argv, callback);
        
        std::cout << "init_done\n";
    }

    ~DetectorOutput(){
        _det->finalize();
        for(auto it = tls.begin(); it != tls.end(); it++){
            delete it->second;
        }
        std::cout << "finished";
    }

    void makeOutput(std::shared_ptr<ipc::event::BufferEntry> buf){
        //std::cout << "do_stuff\n";
        switch(buf->type){
            case ipc::event::Type::FUNCENTER:
                _det->func_enter(
                    tls[buf->payload.funcenter.thread_id],
                    &(buf->payload.funcenter.pc)
                    );
                break;
            case ipc::event::Type::FUNCEXIT:
                _det->func_exit(tls[buf->payload.funcexit.thread_id]);
                break;
            case ipc::event::Type::MEMREAD:
                _det->read(
                    tls[buf->payload.memaccess.thread_id],
                    &(buf->payload.memaccess.pc),
                    &(buf->payload.memaccess.addr),
                    buf->payload.memaccess.size
                );
                break;
            case ipc::event::Type::MEMWRITE:
                _det->write(
                    tls[buf->payload.memaccess.thread_id],
                    &(buf->payload.memaccess.pc),
                    &(buf->payload.memaccess.addr),
                    buf->payload.memaccess.size
                );
                break;
            case ipc::event::Type::ACQUIRE:
                _det->acquire(
                     tls[buf->payload.mutex.thread_id],
                     &(buf->payload.mutex.addr),
                     buf->payload.mutex.recursive,
                     buf->payload.mutex.write
                );
                break;
            case ipc::event::Type::RELEASE:
                _det->release(
                     tls[buf->payload.mutex.thread_id],
                     &(buf->payload.mutex.addr),
                     buf->payload.mutex.write
                );
                break;
            case ipc::event::Type::HAPPENSBEFORE:
                _det->happens_before(
                    tls[buf->payload.happens.thread_id],
                    &(buf->payload.happens.id)
                );
                break;
            case ipc::event::Type::HAPPENSAFTER:
                _det->happens_after(
                    tls[buf->payload.happens.thread_id],
                    &(buf->payload.happens.id)
                );
                break;
            case ipc::event::Type::ALLOCATION:
                _det->allocate(
                    tls[buf->payload.allocation.thread_id],
                    &(buf->payload.allocation.pc),
                    &(buf->payload.allocation.addr),
                    buf->payload.allocation.size
                );
                break;
            case ipc::event::Type::FREE:
                _det->deallocate(
                    tls[buf->payload.allocation.thread_id],
                    &(buf->payload.allocation.addr)
                );
                break;
            case ipc::event::Type::FORK:
                std::cout << "fork\n";
                fork(buf->payload.forkjoin.child, buf->payload.forkjoin.parent);
                break;
            case ipc::event::Type::JOIN:
                std::cout << "join\n";
                /*_det->finish(
                    tls[buf->payload.forkjoin.child],
                    buf->payload.forkjoin.child
                );*/
                _det->join(
                    buf->payload.forkjoin.parent,
                    buf->payload.forkjoin.child
                );
                delete tls[buf->payload.forkjoin.child];  //delete 'local' tls
                break;
            case ipc::event::Type::DETACH:
                _det->detach(
                    tls[buf->payload.detachfinish.thread_id],
                    buf->payload.detachfinish.thread_id
                );
                break;
            case ipc::event::Type::FINISH:
                _det->finish(
                    tls[buf->payload.detachfinish.thread_id],
                    buf->payload.detachfinish.thread_id
                );
                delete tls[buf->payload.forkjoin.child]; //delete 'local' tls
                break;
        }
    }

    static void callback(const Detector::Race* race){
        std::cout << "race";
    }
};

#endif //DETECTOR_OUTPUT_H