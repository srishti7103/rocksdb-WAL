CXX = g_++
CXXFLAGS = -std=c++17 -I../include -O2
LDFLAGS = -L.. -lrocksdb -lpthread -lsnappy -lzstd -llz4 -lbz2 -lz -ldl

TARGETS = sync_bench fragment_bench recovery_bench batch_bench skew_bench

all: $(TARGETS)

sync_bench: sync_bench.cc
	$(CXX) $(CXXFLAGS) $@.cc -o $@ $(LDFLAGS)

fragment_bench: fragment_bench.cc
	$(CXX) $(CXXFLAGS) $@.cc -o $@ $(LDFLAGS)

recovery_bench: recovery_bench.cc
	$(CXX) $(CXXFLAGS) $@.cc -o $@ $(LDFLAGS)

batch_bench: batch_bench.cc
	$(CXX) $(CXXFLAGS) $@.cc -o $@ $(LDFLAGS)

skew_bench: skew_bench.cc
	$(CXX) $(CXXFLAGS) $@.cc -o $@ $(LDFLAGS)

clean:
	rm -f $(TARGETS)
