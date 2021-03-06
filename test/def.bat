@echo off

::   Debug   Release

:: -d ast_tree,als_stack --print ir 
I:\GitHub\Def\x64\Debug\def "index.def" -d ast_tree --print ir --emit obj -o index.obj

:: -d ast_tree,als_stack,tok_list
:: -d prepare_words,binding_spread,mulmcr_words
:: --print ir
:: --emit ast -o index.txt
:: --emit ir -o index.ll
:: --emit asm -o index.s
:: --emit obj -o index.obj
:: -d ast_tree,als_stack,tok_list


:: llc index.ll
:: llc -filetype=obj index.ll
:: gcc index.s -o index.exe
gcc index.obj "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\lib\chkstk.obj" -o index.exe


echo --------
index.exe
echo.
echo --------

