import qbs 

Product {
    
    name: "qzebradev"
    type: ["staticlibrary"]
    
    Depends { name: "cpp" }
    Depends { name: "Qt"; submodules: ['core', 'core-private'] }
    
    files: [
        '**/*.cpp',
        '**/*.h',
    ]
}

