TARGETS = common
all: $(TARGETS)

common: EmailRepository.proto
	protoc --grpc_out=. --plugin=protoc-gen-grpc='/usr/local/bin/grpc_cpp_plugin' $^
	protoc --cpp_out=. $^
	
clean:
	rm -fv *.o *.pb.cc *.pb.h $(TARGETS) *~