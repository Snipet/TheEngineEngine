#!/bin/sh
# Writes a copy of host/src/Program.cpp whose FMinus options can be overridden by environment variables, for
# measurement only (qbench links it ahead of libtee_host.a; host/ itself is never changed):
#   TEE_Q_OPT=0..3  TEE_Q_MATH=vector|libm|fast  TEE_Q_DENORM=flush|preserve  TEE_Q_KNOBS=n  TEE_Q_IR=<file>
#   TEE_Q_DEFINES=NAME[=value];NAME...   (macros for the program's #ifdef: TEE_FAST, the fast switch)
set -e
src="$1/host/src/Program.cpp"; out="$2"
awk '
/o.boundsCheck = options.checks;/ {
  print; print "        if (const char* e = std::getenv(\"TEE_Q_OPT\")) o.optimisation = std::atoi(e);";
  print "        if (const char* e = std::getenv(\"TEE_Q_MATH\")) o.math = std::string(e) == \"libm\" ? fminus::Program::Math::Libm : std::string(e) == \"fast\" ? fminus::Program::Math::Fast : fminus::Program::Math::Vector;";
  print "        if (const char* e = std::getenv(\"TEE_Q_DENORM\")) o.denormals = std::string(e) == \"preserve\" ? fminus::Program::Denormals::Preserve : fminus::Program::Denormals::Flush;";
  print "        if (const char* e = std::getenv(\"TEE_Q_KNOBS\")) o.knobVersions = (unsigned)std::atoi(e);";
  print "        if (std::getenv(\"TEE_Q_IR\")) o.keepIr = fminus::Program::KeepIr::Optimised;";
  print "        if (const char* e = std::getenv(\"TEE_Q_DEFINES\")) { std::string all(e); std::size_t at = 0; while (at < all.size()) { std::size_t end = all.find(\";\", at); if (end == std::string::npos) end = all.size(); const std::string one = all.substr(at, end - at); const std::size_t eq = one.find(\"=\"); if (!one.empty()) o.defines[one.substr(0, eq)] = eq == std::string::npos ? \"1\" : one.substr(eq + 1); at = end + 1; } }";
  next }
/seconds = secondsSince\(start\);/ && !done {
  print; print "        if (const char* e = std::getenv(\"TEE_Q_IR\")) if (program) { std::ofstream f(e); f << program->optimisedIr(); }"; done=1; next }
{ print }' "$src" | sed 's|#include <chrono>|#include <chrono>\n#include <cstdlib>|' > "$out"
