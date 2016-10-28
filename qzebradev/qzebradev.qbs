import qbs 

Product {
    
    name: "qzebradev"
    type: ["staticlibrary"]
    
    Depends { name: "cpp" }
    Depends { name: "Qt"; submodules: ['core'] }
    
    files: [
        '**/*.cpp',
        '**/*.h'
    ]
}

