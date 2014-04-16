// RAPIDJSON_ASSERT
// throw exceptions instead of assert
#ifndef RAPIDJSON_ASSERT
#define RAPIDJSON_ASSERT(X) do { if (!(X)) {throw(false); } } while(0)
#endif

