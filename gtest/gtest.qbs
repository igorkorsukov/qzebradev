import qbs 

Product {
    
    name: "gtest"
    type: ["staticlibrary"]
    
    Depends { name: "cpp" }

    cpp.includePaths: ['./', './include']
    
    files: [
        '**/*.cc',
        '**/*.h'
    ]
}

