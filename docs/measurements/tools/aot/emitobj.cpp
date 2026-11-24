// docs/measurements/tools/aot/emitobj.cpp: for q8-aot.md. Reads the optimised LLVM IR FMinus keeps (qbench's
// TEE_Q_IR: the module the JIT generates machine code from) and writes an object file with LLVM's code generator at
// -O2, for this host's CPU or for a named one (--cpu x86-64-v3, its features then replaced): how long fminus -c's
// code generation takes on sim/, how big the object is, and what it exports and needs.
//   emitobj <in.ll> <out.o> [cpu]
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>

#include <chrono>
#include <cstdio>

int main(int argc, char** argv)
{
    if (argc < 3)
        return 2;
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::LLVMContext ctx;
    llvm::SMDiagnostic err;
    const auto t0 = std::chrono::steady_clock::now();
    std::unique_ptr<llvm::Module> m = llvm::parseIRFile(argv[1], err, ctx);
    if (!m)
    {
        err.print("emitobj", llvm::errs());
        return 1;
    }
    const auto t1 = std::chrono::steady_clock::now();
    const std::string cpu = argc > 3 ? argv[3] : llvm::sys::getHostCPUName().str();
    if (argc > 3)
        for (llvm::Function& f : *m)
        {
            f.removeFnAttr("target-features");
            f.removeFnAttr("target-cpu");
            f.addFnAttr("target-cpu", cpu);
        }
    std::string e;
    const llvm::Triple triple(m->getTargetTriple());
    const llvm::Target* t = llvm::TargetRegistry::lookupTarget(triple, e);
    if (!t)
    {
        std::fprintf(stderr, "%s\n", e.c_str());
        return 1;
    }
    llvm::TargetOptions opt;
    std::unique_ptr<llvm::TargetMachine> tm(t->createTargetMachine(triple, cpu, "", opt, llvm::Reloc::PIC_,
                                                                    std::nullopt, llvm::CodeGenOptLevel::Default));
    m->setDataLayout(tm->createDataLayout());
    std::error_code ec;
    llvm::raw_fd_ostream out(argv[2], ec, llvm::sys::fs::OF_None);
    llvm::legacy::PassManager pm;
    if (tm->addPassesToEmitFile(pm, out, nullptr, llvm::CodeGenFileType::ObjectFile))
        return 1;
    pm.run(*m);
    out.flush();
    const auto t2 = std::chrono::steady_clock::now();
    std::printf("parse %.2f s, code generation %.2f s, cpu %s\n", std::chrono::duration<double>(t1 - t0).count(),
                std::chrono::duration<double>(t2 - t1).count(), cpu.c_str());
    return 0;
}
