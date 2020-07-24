/******************************************************************************
 * Copyright (c) 2020, Chen Fang <mtdcy.chen@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************/


// File:    Process.cpp
// Author:  mtdcy.chen
// Changes:
//          1. 20160701     initial version
//

#define LOG_TAG "Process"
#define LOG_NDEBUG 0
#include "Log.h"
#include "Process.h"

#include "Looper.h"
#include "String.h"
#include "Mutex.h"

#include "../stl/List.h"

#include <unistd.h>
#include <signal.h>     // kill

__BEGIN_NAMESPACE_ABE

struct ProcessDescription {
    const String            mCommand;
    const Process::ePolicy  mPolicy;
    pid_t                   mPID;
    
    ProcessDescription(const String& command, const Process::ePolicy policy = Process::kOnce) :
    mCommand(command), mPolicy(policy) { }
};

struct ProcessReceipts : public SharedObject {
    pid_t               mPID;
    
    ProcessReceipts(pid_t pid) : mPID(pid) { }
    
    virtual void onLastRetain() {
        // https://stackoverflow.com/questions/14110738/how-to-terminate-a-child-process-which-is-running-another-program-by-doing-exec
        DEBUG("kill process %d", mPID);
        // step 1: tell child to terminate gracefully
        kill(mPID, SIGTERM);
        // step 2:
        Int status;
        waitpid(mPID, &status, WNOHANG);
        DEBUG("status %d", status);
        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            // NOTHING
            DEBUG("KILLED %d", mPID);
        } else {
            WARN("KILL %d by force", mPID);
            kill(mPID, SIGKILL);
        }
    }
};

static sp<ProcessReceipts> NewProcess(ProcessDescription& desc) {
    pid_t child = ::fork();
    
    if (child == -1) {
        ERROR("fork child %s failed", desc.mCommand.c_str());
        return Nil;
    } else if (child) {    // parent
        DEBUG("fork child %s success", desc.mCommand.c_str());
        return new ProcessReceipts(child);
    } else {        // child
        DEBUG("exec %s", desc.mCommand.c_str());
        Int ret;
        for (;;) {
            ret = execl("/bin/sh", "sh", "-c", desc.mCommand.c_str(), (Char *)Nil);
        }
        DEBUG("terminate child %s", desc.mCommand.c_str());
        _exit(ret);
    }
}

Process::Process() : SharedObject(FOURCC('proc')) {
}

sp<Process> Process::Create(const String& command, ePolicy policy) {
    ProcessDescription desc ( command, policy );
    sp<ProcessReceipts> receipts = NewProcess(desc);
    
    if (receipts.isNil()) {
        ERROR("create process for %s failed", command.c_str());
        return Nil;
    }
    DEBUG("new process %s", command.c_str());
    sp<Process> process = new Process;
    process->mPartner = receipts->RetainObject();
    return process;
}

void Process::onFirstRetain() {
    DEBUG("onFirstRetain");
}

void Process::onLastRetain() {
    DEBUG("onLastRetain");
    mPartner->ReleaseObject();
}

__END_NAMESPACE_ABE
