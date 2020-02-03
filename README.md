# <img title="Automaton" width="231" height="39" src="media/automaton-logo-black-on-white-8x8.svg">
<!-- ALL-CONTRIBUTORS-BADGE:START - Do not remove or modify this section -->
[![All Contributors](https://img.shields.io/badge/all_contributors-4-orange.svg?style=flat-square)](#contributors-)
<!-- ALL-CONTRIBUTORS-BADGE:END -->

## Build Status

<img src="https://ci.appveyor.com/api/projects/status/5euy83253gjqdasg/branch/master?svg=true">

[![Build Status Travis](https://travis-ci.org/automaton-network/automaton.svg?branch=master)](https://travis-ci.org/automaton-network/automaton)


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

### Prerequisites

```
sudo apt-get install build-essential git curl cmake autotools-dev autoconf autogen automake libtool
```

* Git
* curl
* CMake
* g++
* autotools
* libtool

### External Third Party Libraries

First you need to run a script to download and build some required third party libraries.

**Linux & Mac OS**

```
./download_and_build_third_party_libs.sh
```

**Windows**

1. Start [Developer Command Prompt for Visual Studio 2017](https://docs.microsoft.com/en-us/dotnet/framework/tools/developer-command-prompt-for-vs)
2. In the started command prompt type in ``powershell``
3. Move to the project's directory ``cd "<path_to_project>"``
4. Run ``.\download_and_build_third_party_libs_windows.ps1``
5. Build and run with Bazel

### Bazel Build & Run

Once you have bazel you can run the following to build and run Automaton Core

**Linux & Mac OS**
```
./build-and-run-automaton-core.sh
```
**Windows**
```
.\build-and-run-automaton-core.ps1
```

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
    <td align="center"><a href="https://github.com/akovachev"><img src="https://avatars1.githubusercontent.com/u/3320144?v=4" width="100px;" alt=""/><br /><sub><b>Asen Kovachev</b></sub></a><br /><a href="https://github.com/automaton-network/automaton/commits?author=akovachev" title="Code">💻</a> <a href="#projectManagement-akovachev" title="Project Management">📆</a></td>
    <td align="center"><a href="https://github.com/karonka"><img src="https://avatars1.githubusercontent.com/u/7265999?v=4" width="100px;" alt=""/><br /><sub><b>karonka</b></sub></a><br /><a href="https://github.com/automaton-network/automaton/commits?author=karonka" title="Code">💻</a></td>
    <td align="center"><a href="https://github.com/Plazmock"><img src="https://avatars2.githubusercontent.com/u/7255538?v=4" width="100px;" alt=""/><br /><sub><b>Samir Muradov</b></sub></a><br /><a href="https://github.com/automaton-network/automaton/commits?author=Plazmock" title="Code">💻</a></td>
    <td align="center"><a href="https://github.com/ElCampeon"><img src="https://avatars1.githubusercontent.com/u/26015813?v=4" width="100px;" alt=""/><br /><sub><b>ElCampeon</b></sub></a><br /><a href="#business-ElCampeon" title="Business development">💼</a> <a href="#ideas-ElCampeon" title="Ideas, Planning, & Feedback">🤔</a> <a href="#content-ElCampeon" title="Content">🖋</a> <a href="#financial-ElCampeon" title="Financial">💵</a></td>
  </tr>
</table>

<!-- markdownlint-enable -->
<!-- prettier-ignore-end -->
<!-- ALL-CONTRIBUTORS-LIST:END -->

This project follows the [all-contributors](https://github.com/all-contributors/all-contributors) specification. Contributions of any kind welcome!