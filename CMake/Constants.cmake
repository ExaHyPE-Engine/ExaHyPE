list(APPEND BUILD_TYPES "All" "Debug" "Release" "Asserts" "Trace" "Stats" "")
list(TRANSFORM BUILD_TYPES TOLOWER OUTPUT_VARIABLE BUILD_TYPES_LOWER)
list(APPEND PARALLELISM_TYPES "no" "cpp" "omp" "tbb")
list(APPEND GPU_SUPPORT_TYPES "no" "cpp" "omp" "tbb")
list(APPEND GPU_ARCHITECTURE_TYPES
	"sm_20"
	"sm_30"
	"sm_35"
	"sm_37"
	"sm_50"
	"sm_53"
	"sm_52"
	"sm_60"
	"sm_61"
	"sm_62"
	"sm_70"
	"sm_72"
	"sm_75"
	"sm_80"
	"sm_86"
	"sm_90"
	"gfx906"
	"gfx908"
	"gfx90a"
)
list(APPEND DIMENSION_TYPES "2" "3" "2 and 3")

# Some Toolchains of Peano do not work (even for autoconf) therefore the current toolchain options are disabled for CMake
# list(APPEND TOOLCHAIN_TYPES "no" "intel" "nvidia" "scorep" "chrome" "gnu")