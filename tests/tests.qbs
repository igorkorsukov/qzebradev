import qbs

Application {

    name: "tests"

    Depends { name: "cpp" }
    Depends { name: "Qt"; submodules: [ 'core'] }
    Depends { name: "gtest" }
    Depends { name: "qzebradev" }

    targetName: "qzebradev_tests"
    consoleApplication: true

    cpp.cxxLanguageVersion: "c++11"
    cpp.includePaths: ['../', '../gtest/include']

    Group {
        name: "The App itself"
        fileTagsFilter: "application"
        qbs.install: true
        qbs.installDir: "bin"
    }

    files: [
        "**/*.cpp",
        "**/*.h"
    ]
}
