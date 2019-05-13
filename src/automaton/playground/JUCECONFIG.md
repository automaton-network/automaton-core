# JUCE Configuration

Configuring JUCE in order to link to Automaton Core libraries is a bit of a challenge. This is not the best solution, but it is a solution.

Here are some of the configurations that we're modifying and plugging into the jucer configuration.

## Mac OS X

### Extra Preprocessor Definitions

```
ELPP_NO_LOG_TO_FILE
ELPP_NO_DEFAULT_LOG_FILE
ELPP_FEATURE_PERFORMANCE_TRACKING
ELPP_FEATURE_CRASH_LOG
ELPP_THREAD_SAFE
ELPP_STL_LOGGING
```

### Extra Compiler Flags

```
-I $(SRCROOT)/../../../../..
-I $(SRCROOT)/../../../../../bazel-src/external/com_google_protobuf/src
-I $(SRCROOT)/../../../../../local_third_party/boost
-I $(SRCROOT)/../../../../../local_third_party/cryptopp
-I $(SRCROOT)/../../../../../local_third_party/easyloggingpp/src
-I $(SRCROOT)/../../../../../local_third_party/lua
-I $(SRCROOT)/../../../../../local_third_party/sol2/single/sol
```

### Extra Linker Flags

```
$(SRCROOT)/../../../../../bazel-bin/automaton/core/cli/libcli.a
$(SRCROOT)/../../../../../bazel-bin/automaton/core/common/libstatus.a
$(SRCROOT)/../../../../../bazel-bin/automaton/core/crypto/cryptopp/libcryptopp.a
$(SRCROOT)/../../../../../bazel-bin/automaton/core/crypto/ed25519_orlp/libed25519_orlp.a
$(SRCROOT)/../../../../../bazel-bin/automaton/core/crypto/libcrypto.a
$(SRCROOT)/../../../../../bazel-bin/automaton/core/data/libdata.a
$(SRCROOT)/../../../../../bazel-bin/automaton/core/data/protobuf/libprotobuf.a
$(SRCROOT)/../../../../../bazel-bin/automaton/core/io/libio.a
$(SRCROOT)/../../../../../bazel-bin/automaton/core/network/libhttp_server.a
$(SRCROOT)/../../../../../bazel-bin/automaton/core/network/libnetwork_tcp.a
$(SRCROOT)/../../../../../bazel-bin/automaton/core/network/libnetwork.a
$(SRCROOT)/../../../../../bazel-bin/automaton/core/network/libsimulated_connection.a
$(SRCROOT)/../../../../../bazel-bin/automaton/core/node/libnode.a
$(SRCROOT)/../../../../../bazel-bin/automaton/core/node/lua_node/liblua_node.a
$(SRCROOT)/../../../../../bazel-bin/automaton/core/script/libscript.a
$(SRCROOT)/../../../../../bazel-bin/automaton/core/smartproto/libsmartproto.a
$(SRCROOT)/../../../../../bazel-bin/automaton/core/state/libstate.a
$(SRCROOT)/../../../../../bazel-bin/automaton/core/storage/libblobstore.a
$(SRCROOT)/../../../../../bazel-bin/automaton/core/storage/libpersistent_blobstore.a
$(SRCROOT)/../../../../../bazel-bin/automaton/core/storage/libpersistent_storage.a
$(SRCROOT)/../../../../../bazel-bin/automaton/core/testnet/libtestnet.a
$(SRCROOT)/../../../../../bazel-bin/external/com_google_protobuf/libprotobuf_lite.a
$(SRCROOT)/../../../../../bazel-bin/external/com_google_protobuf/libprotobuf.a
$(SRCROOT)/../../../../../bazel-bin/external/cryptopp/libcryptopp.a
$(SRCROOT)/../../../../../bazel-bin/external/easyloggingpp/libeasyloggingpp.a
$(SRCROOT)/../../../../../bazel-bin/external/ed25519_orlp/libed25519_orlp.a
$(SRCROOT)/../../../../../bazel-bin/external/json/libjson.a
$(SRCROOT)/../../../../../bazel-bin/external/lua/libliblua.a
$(SRCROOT)/../../../../../bazel-bin/external/sol/libsol.a
$(SRCROOT)/../../../../../local_third_party/boost/stage/lib/libboost_iostreams.a
$(SRCROOT)/../../../../../local_third_party/boost/stage/lib/libboost_system.a
$(SRCROOT)/../../../../../local_third_party/boost/stage/lib/libboost_filesystem.a
