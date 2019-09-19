#
# Copyright 2015 gRPC authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

HOST_SYSTEM = $(shell uname | cut -f 1 -d_)
SYSTEM ?= $(HOST_SYSTEM)
CXX = g++
CPPFLAGS += `pkg-config --cflags protobuf grpc`
CXXFLAGS += -std=c++14
ifeq ($(SYSTEM),Darwin)
LDFLAGS += -L/usr/local/lib `pkg-config --libs protobuf grpc++`\
           -lgrpc++_reflection\
           -ldl
else
LDFLAGS += -L/usr/local/lib `pkg-config --libs protobuf grpc++`\
           -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed\
           -ldl
endif
PROTOC = protoc
GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH ?= `which $(GRPC_CPP_PLUGIN)`

PROTOS_PATH = ../common

TARGETS = bigtable

vpath %.proto $(PROTOS_PATH)

all: system-check sample master $(TARGETS)

bigtable: ../common/EmailRepository.pb.o ../common/EmailRepository.grpc.pb.o EmailRepositoryImpl.o\
		main.cpp BigTable.cpp
	$(CXX) -std=c++14 $^ $(LDFLAGS) -L/opt/local/bin/openssl -lcrypto -lboost_system -lboost_filesystem -lboost_thread -g -o $@

# userRepository: ../common/UserRepository.pb.o ../common/UserRepository.grpc.pb.o UserRepositoryImpl.o
# 	$(CXX) $^ $(LDFLAGS) -o $@

fakeMaster: ../common/WriteService.pb.o ../common/WriteService.grpc.pb.o ../common/WriteServiceClient.o\
	BigTable.o fakeMaster.o Logger.o
	$(CXX) -std=c++14 $^ $(LDFLAGS) -L/opt/local/bin/openssl -lcrypto -lboost_system -lboost_filesystem -lboost_thread -g -o $@

sample: ../common/UserRepository.pb.o ../common/UserRepository.grpc.pb.o UserRepositoryImpl.o\
		../common/EmailRepository.pb.o ../common/EmailRepository.grpc.pb.o EmailRepositoryImpl.o\
		../common/StorageRepository.pb.o ../common/StorageRepository.grpc.pb.o StorageRepositoryImpl.o\
		../common/AdminConsoleRepository.pb.o ../common/AdminConsoleRepository.grpc.pb.o AdminConsoleRepositoryImpl.o\
		../common/HeartbeatService.pb.o ../common/HeartbeatService.grpc.pb.o ../common/HeartbeatServiceImpl.o\
		../common/WriteService.pb.o ../common/WriteService.grpc.pb.o ../common/WriteServiceImpl.o ../common/WriteServiceClient.o\
		../common/MasterService.pb.o ../common/MasterService.grpc.pb.o ../common/MasterServiceImpl.o\
		../common/ServerToMaster.pb.o ../common/ServerToMaster.grpc.pb.o ../common/ServerToMasterImpl.o ../common/ServerToMasterClient.o\
		../common/ServerToServer.pb.o ../common/ServerToServer.grpc.pb.o ../common/ServerToServerImpl.o ../common/ServerToServerClient.o\
		BigTable.o SampleRpcServer.o Logger.o
	$(CXX) -std=c++14 $^ $(LDFLAGS) -L/opt/local/bin/openssl -lcrypto -lboost_system -lboost_filesystem -lboost_thread -g -o $@
	# $(CXX) $^ $(LDFLAGS) -lboost_system -lboost_filesystem -lboost_thread -g -o $@
	
master: ../common/HeartbeatService.pb.o ../common/HeartbeatService.grpc.pb.o ../common/HeartbeatServiceImpl.o ../common/HeartbeatServiceClient.o\
		../common/WriteService.pb.o ../common/WriteService.grpc.pb.o ../common/WriteServiceClient.o\
		../common/MasterService.pb.o ../common/MasterService.grpc.pb.o ../common/MasterServiceImpl.o\
		../common/ServerToMaster.pb.o ../common/ServerToMaster.grpc.pb.o ../common/ServerToMasterImpl.o\
		MasterServer.o
	$(CXX) -std=c++14 $^ $(LDFLAGS) -L/opt/local/bin/openssl -lcrypto -lboost_system -lboost_filesystem -lboost_thread -g -o $@
	
%.grpc.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --grpc_out=../common --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH) $<

%.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --cpp_out=../common $<


clean:
	rm -fv *.o ../common/*.pb.cc ../common/*.pb.h ../common/*.o $(TARGETS) *~ 


# The following is to test your system and ensure a smoother experience.
# They are by no means necessary to actually compile a grpc-enabled software.

PROTOC_CMD = which $(PROTOC)
PROTOC_CHECK_CMD = $(PROTOC) --version | grep -q libprotoc.3
PLUGIN_CHECK_CMD = which $(GRPC_CPP_PLUGIN)
HAS_PROTOC = $(shell $(PROTOC_CMD) > /dev/null && echo true || echo false)
ifeq ($(HAS_PROTOC),true)
HAS_VALID_PROTOC = $(shell $(PROTOC_CHECK_CMD) 2> /dev/null && echo true || echo false)
endif
HAS_PLUGIN = $(shell $(PLUGIN_CHECK_CMD) > /dev/null && echo true || echo false)

SYSTEM_OK = false
ifeq ($(HAS_VALID_PROTOC),true)
ifeq ($(HAS_PLUGIN),true)
SYSTEM_OK = true
endif
endif

system-check:
ifneq ($(HAS_VALID_PROTOC),true)
	@echo " DEPENDENCY ERROR"
	@echo
	@echo "You don't have protoc 3.0.0 installed in your path."
	@echo "Please install Google protocol buffers 3.0.0 and its compiler."
	@echo "You can find it here:"
	@echo
	@echo "   https://github.com/google/protobuf/releases/tag/v3.0.0"
	@echo
	@echo "Here is what I get when trying to evaluate your version of protoc:"
	@echo
	-$(PROTOC) --version
	@echo
	@echo
endif
ifneq ($(HAS_PLUGIN),true)
	@echo " DEPENDENCY ERROR"
	@echo
	@echo "You don't have the grpc c++ protobuf plugin installed in your path."
	@echo "Please install grpc. You can find it here:"
	@echo
	@echo "   https://github.com/grpc/grpc"
	@echo
	@echo "Here is what I get when trying to detect if you have the plugin:"
	@echo
	-which $(GRPC_CPP_PLUGIN)
	@echo
	@echo
endif
ifneq ($(SYSTEM_OK),true)
	@false
endif
