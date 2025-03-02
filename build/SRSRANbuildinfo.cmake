
execute_process(
COMMAND git rev-parse --abbrev-ref HEAD
WORKING_DIRECTORY "/home/zq5820/NG-Scope-SCM-ZMS-NRDZ"
OUTPUT_VARIABLE GIT_BRANCH
OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
COMMAND git log -1 --format=%h
WORKING_DIRECTORY "/home/zq5820/NG-Scope-SCM-ZMS-NRDZ"
OUTPUT_VARIABLE GIT_COMMIT_HASH
OUTPUT_STRIP_TRAILING_WHITESPACE
)

message(STATUS "Generating build_info.h")
configure_file(
  /home/zq5820/NG-Scope-SCM-ZMS-NRDZ/lib/include/srsran/build_info.h.in
  /home/zq5820/NG-Scope-SCM-ZMS-NRDZ/build/lib/include/srsran/build_info.h
)
