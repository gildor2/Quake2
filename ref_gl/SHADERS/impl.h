#define NUM_TOKENS 5

typedef enum
{
	TOK_MAP,
	TOK_CLAMPMAP,
	TOK_ANIMMAP,
	TOK_GL_ONE,
	TOK_GL_ZERO
} shaderToken_t;

static char *tokens[NUM_TOKENS] = {
	"map",
	"clampmap",
	"animMap",
	"GL_ONE",
	"GL_ZERO"
};

