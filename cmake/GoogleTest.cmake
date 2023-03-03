include(FetchContent)

message("Fetching https://github.com/google/googletest.git")
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG origin/main
        GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(googletest)
