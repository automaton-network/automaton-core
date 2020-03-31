# <img title="Automaton" width="231" height="39" src="media/automaton-logo-black-on-white-8x8.svg">

<!-- ALL-CONTRIBUTORS-BADGE:START - Do not remove or modify this section -->
[![All Contributors](https://img.shields.io/badge/all_contributors-10-orange.svg?style=flat-square)](#contributors-)
<!-- ALL-CONTRIBUTORS-BADGE:END -->

## Build Status

[![Automaton Core](https://github.com/automaton-network/automaton/workflows/C/C++%20CI/badge.svg?branch=master)](https://github.com/automaton-network/automaton/actions?query=workflow%3A%22C%2FC%2B%2B+CI%22+branch%3Amaster)
[![Smart Contract](https://github.com/automaton-network/automaton/workflows/Node.js%20CI/badge.svg?branch=master)](https://github.com/automaton-network/automaton/actions?query=workflow%3A%22Node.js+CI%22+branch%3Amaster)

Table of Contents
=================

  * [License](#license)
  * [Building Automaton](#building-automaton)
  * [Getting Started](#getting-started)

## License

#### Everything in this repo is licensed under the MIT License.

Check the [LICENSE](LICENSE) file for more details.

#### 🚨  HIGHLY EXPERIMENTAL! USE AT YOUR OWN RISK!

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

## Contact

Telegram: https://t.me/automaton_network

## Building Automaton

### Linux

At the moment Ubuntu is the only known Linux distro compatible with Automaton.

#### Prerequisites

```
sudo apt-get install build-essential git curl cmake autotools-dev autoconf autogen automake libtool
```

* Git
* curl
* CMake
* g++
* autotools
* libtool

#### Build instructions

From the automaton repo:

```
cd src
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../local_third_party
make
make install
```

make install is necessary in order to setup libraries for projects using core (such as the Automaton Playground)

### Windows

#### Prerequisites

* Visual Studio (VS 2019 recommended)
* CMake (3.15 minimum) https://cmake.org/download/
* Git - This is what we use: https://gitforwindows.org/
* Perl (needed to build OpenSSL) - This is what we use: http://strawberryperl.com/

#### Build instructions

**IMPORTANT FOR WINDOWS**
Sync the repos into a root path that's not too long, otherwise boost filenames might become unaccessible! e.g. c:\dev, instead of 

Start the "x64 Native Tools Command Prompt for VS 2019"

From the automaton repo folder:

```
cd src
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -A x64 -Dautomaton_STATIC_RUNTIME=OFF -DCMAKE_INSTALL_PREFIX=../local_third_party
msbuild /t:Build /m /p:Configuration=Debug /p:Platform=x64 INSTALL.vcxproj
```

This will build and install necessary libraries for projects using core (such as the Automaton Playground)

## Getting Started

*COMING SOON!*

We're getting our step-by-step tutorials ready.

## Contributors ✨

Thanks goes to these wonderful people ([emoji key](https://allcontributors.org/docs/en/emoji-key)):

<!-- ALL-CONTRIBUTORS-LIST:START - Do not remove or modify this section -->
<!-- prettier-ignore-start -->
<!-- markdownlint-disable -->
<table>
  <tr>
    <td align="center"><a href="https://github.com/akovachev"><img src="https://avatars1.githubusercontent.com/u/3320144?v=4" width="100px;" alt=""/><br /><sub><b>Asen Kovachev</b></sub></a><br /><a href="https://github.com/automaton-network/automaton/commits?author=akovachev" title="Code">💻</a> <a href="https://github.com/automaton-network/automaton/pulls?q=is%3Apr+reviewed-by%3Aakovachev" title="Reviewed Pull Requests">👀</a> <a href="#projectManagement-akovachev" title="Project Management">📆</a> <a href="#business-akovachev" title="Business development">💼</a> <a href="#content-akovachev" title="Content">🖋</a> <a href="#financial-akovachev" title="Financial">💵</a> <a href="#ideas-akovachev" title="Ideas, Planning, & Feedback">🤔</a> <a href="#infra-akovachev" title="Infrastructure (Hosting, Build-Tools, etc)">🚇</a> <a href="#platform-akovachev" title="Packaging/porting to new platform">📦</a> <a href="#talk-akovachev" title="Talks">📢</a> <a href="#design-akovachev" title="Design">🎨</a></td>
    <td align="center"><a href="https://github.com/ElCampeon"><img src="https://avatars1.githubusercontent.com/u/26015813?v=4" width="100px;" alt=""/><br /><sub><b>ElCampeon</b></sub></a><br /><a href="#business-ElCampeon" title="Business development">💼</a> <a href="#ideas-ElCampeon" title="Ideas, Planning, & Feedback">🤔</a> <a href="#content-ElCampeon" title="Content">🖋</a> <a href="#financial-ElCampeon" title="Financial">💵</a> <a href="#design-ElCampeon" title="Design">🎨</a> <a href="#fundingFinding-ElCampeon" title="Funding Finding">🔍</a></td>
    <td align="center"><a href="https://github.com/karonka"><img src="https://avatars1.githubusercontent.com/u/7265999?v=4" width="100px;" alt=""/><br /><sub><b>karonka</b></sub></a><br /><a href="https://github.com/automaton-network/automaton/commits?author=karonka" title="Code">💻</a></td>
    <td align="center"><a href="https://github.com/Plazmock"><img src="https://avatars2.githubusercontent.com/u/7255538?v=4" width="100px;" alt=""/><br /><sub><b>Samir Muradov</b></sub></a><br /><a href="https://github.com/automaton-network/automaton/commits?author=Plazmock" title="Code">💻</a></td>
    <td align="center"><a href="https://github.com/zorancvetkov"><img src="https://avatars0.githubusercontent.com/u/5655655?v=4" width="100px;" alt=""/><br /><sub><b>zorancvetkov</b></sub></a><br /><a href="#ideas-zorancvetkov" title="Ideas, Planning, & Feedback">🤔</a></td>
    <td align="center"><a href="http://iohk.io"><img src="https://avatars0.githubusercontent.com/u/6830638?v=4" width="100px;" alt=""/><br /><sub><b>Richard Wild</b></sub></a><br /><a href="#design-RichardWild001" title="Design">🎨</a> <a href="#ideas-RichardWild001" title="Ideas, Planning, & Feedback">🤔</a> <a href="#content-RichardWild001" title="Content">🖋</a></td>
    <td align="center"><a href="https://github.com/MartinPenkov"><img src="https://avatars2.githubusercontent.com/u/26440634?v=4" width="100px;" alt=""/><br /><sub><b>MartinPenkov</b></sub></a><br /><a href="https://github.com/automaton-network/automaton/commits?author=MartinPenkov" title="Code">💻</a></td>
  </tr>
  <tr>
    <td align="center"><a href="https://github.com/sept-en"><img src="https://avatars1.githubusercontent.com/u/920587?v=4" width="100px;" alt=""/><br /><sub><b>sept-en</b></sub></a><br /><a href="https://github.com/automaton-network/automaton/commits?author=sept-en" title="Code">💻</a></td>
    <td align="center"><a href="https://github.com/vitalyster"><img src="https://avatars2.githubusercontent.com/u/1052407?v=4" width="100px;" alt=""/><br /><sub><b>vitalyster</b></sub></a><br /><a href="#infra-vitalyster" title="Infrastructure (Hosting, Build-Tools, etc)">🚇</a></td>
    <td align="center"><a href="https://github.com/todddorov"><img src="https://avatars1.githubusercontent.com/u/62315265?v=4" width="100px;" alt=""/><br /><sub><b>todddorov</b></sub></a><br /><a href="#design-todddorov" title="Design">🎨</a></td>
  </tr>
</table>

<!-- markdownlint-enable -->
<!-- prettier-ignore-end -->
<!-- ALL-CONTRIBUTORS-LIST:END -->

This project follows the [all-contributors](https://github.com/all-contributors/all-contributors) specification. Contributions of any kind welcome!
