set extentions=(*.cc,*.h)
set options=--filter=-legal/copyright,-build/header_guard,-build/c++11

set dir=%cd%\crypto
for /r %dir% %%v in %extentions% do cpplint %options% %%v

set dir=%cd%\data
for /r %dir% %%v in %extentions% do cpplint %options% %%v

set dir=%cd%\network
for /r %dir% %%v in %extentions% do cpplint %options% %%v

set dir=%cd%\state
for /r %dir% %%v in %extentions% do cpplint %options% %%v

set dir=%cd%\storage
for /r %dir% %%v in %extentions% do cpplint %options% %%v

set dir=%cd%\tests
for /r %dir% %%v in %extentions% do cpplint %options% %%v
