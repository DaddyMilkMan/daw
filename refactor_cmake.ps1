$files = Get-ChildItem "apps\desktop\Source" -Recurse -Filter "CMakeLists.txt"
foreach ($file in $files) {
    $content = Get-Content $file.FullName -Raw
    $content = $content -replace "target_sources\(ZenithDAW", "target_sources(ZenithDAW_lib"
    $content = $content -replace "target_include_directories\(ZenithDAW", "target_include_directories(ZenithDAW_lib"
    $content = $content -replace "target_link_libraries\(ZenithDAW", "target_link_libraries(ZenithDAW_lib"
    Set-Content $file.FullName $content
    Write-Host "Updated $($file.FullName)"
}
