idf_component_register(
	SRCS
	"main.c"
	"unicode.c"
	INCLUDE_DIRS
	"."
	EMBED_TXTFILES "Ubuntu-R.ttf"
)
target_compile_options(${COMPONENT_LIB} PRIVATE -O3)
