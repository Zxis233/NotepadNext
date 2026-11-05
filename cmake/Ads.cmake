# Share the same patched ADS dependency between the application and tab tests.
include("${CMAKE_CURRENT_LIST_DIR}/CPM.cmake")

set(ads_wrap_patch "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/patches/ads-wrap-tabs.patch")
file(SHA256 "${ads_wrap_patch}" ads_wrap_patch_hash)
CPMAddPackage(
    NAME ads
    VERSION 5.1.1
    GITHUB_REPOSITORY githubuser0xFFFF/Qt-Advanced-Docking-System
    GIT_TAG 023ce95934fecfd4cf5c672c9c404fabe0f54923 # v5.1.1
    # A changed patch must not reuse a previously patched CPM source cache.
    CUSTOM_CACHE_KEY "v5.1.1-wrap-${ads_wrap_patch_hash}"
    PATCHES "${ads_wrap_patch}"
    OPTIONS
        "BUILD_STATIC ON"
        "BUILD_EXAMPLES OFF"
)
target_include_directories(qtadvanceddocking-qt6 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../src/widgets")
