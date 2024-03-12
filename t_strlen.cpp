#line 216 "README.md"
#include "solid/require.h"
#line 46
#include "strlen.h"

#line 238
void test_null_strlen() {
	bool got_exception { false };
	try {
#line 395
		std::ignore = strlen(String_Literal { nullptr });
#line 242
	} catch (const solid::require::Error&) { got_exception = true; }
	require(got_exception);
}
#line 48
int main() {
#line 404
	require(strlen(String_Literal { "" }) == 0);
	require(strlen(String_Literal { "abc" }) == 3);
	require(strlen(String_Literal { "a\0b" }) == 1);
	test_null_strlen();
#line 52
}
