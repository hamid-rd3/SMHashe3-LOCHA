########################################
# High-resolution timing functions
########################################

checkCachedVarsDepend(TIMING "high-resolution timing functions")

# Tell findVariant() where to store the files and variables it touches
set(VARIANT_VARLISTVAR  "TIMING_VARLIST")
set(VARIANT_FILELISTVAR "TIMING_FILELIST")

set(MONOTONIC_VARIANTS
  "monotonic clock"
  "mclock"
  "@FIXEDINT_IMPL@\n@FORCE_INLINE_IMPL@\n"
  3
)
findVariant(MONOTONIC)

set(TIMING_VARIANTS
  "hardware performance counter"
  "timing"
  "@FIXEDINT_IMPL@\n@FORCE_INLINE_IMPL@\n@MONOTONIC_IMPL@\n"
  8
)
findVariant(TIMING)

# By depending on this .cmake file, the cache will be cleared if the
# list of files were to ever change, as this file is the only one that
# can change it.
list(APPEND TIMING_FILELIST "${DETECT_DIR}/timing.cmake")

setCachedVarsDepend(TIMING TIMING_VARLIST TIMING_FILELIST)
