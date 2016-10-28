import qbs

Application {

    name: "app"

    Depends { name: "cpp" }
    Depends { name: "Qt"; submodules: [ 'core'] }
    Depends { name: "qzebradev" }

    targetName: "qzebradev_testapp"
    consoleApplication: true

    cpp.cxxLanguageVersion: "c++11"
    cpp.includePaths: ['../']

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
