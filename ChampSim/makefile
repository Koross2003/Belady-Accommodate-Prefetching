
hawkeye:compile_hawkeye
	./hawkeye-config2 -warmup_instructions 5000000 -simulation_instructions 200000000 -traces trace/bzip2_10M.trace > single_core_result/hawkeye_single_core.txt
homony:compile_homony
	./homony-config2 -warmup_instructions 5000000 -simulation_instructions 200000000 -traces trace/bzip2_10M.trace > single_core_result/homony_single_core.txt
harmony:compile_harmony
	./harmony-config2 -warmup_instructions 5000000 -simulation_instructions 200000000 -traces trace/bzip2_10M.trace > single_core_result/harmony_single_core.txt

compile_hawkeye:
	g++ -Wall --std=c++11 -o hawkeye-config2 example/hawkeye/hawkeye.cc lib/config2.a

compile_homony:
	g++ -Wall --std=c++11 -o homony-config2 example/homony/homony.cc lib/config2.a

compile_harmony:
	g++ -Wall --std=c++11 -o harmony-config2 example/harmony/harmony.cc lib/config2.a


SOURCES := $(wildcard trace/*.gz)
OUTPUTS := $(patsubst trace/%.gz,single_core_result/%.txt,$(SOURCES))

single: $(OUTPUTS)

single_core_result/%.txt: trace/%.gz compile_single_core
	./harmony_lru-config2 -warmup_instructions 5000000 -simulation_instructions 200000000 -traces $< > $@
multiple_core_result: comile_multiple_core
	./harmony_lru-config4 -warmup_instructions 1000000 -simulation_instructions 1000000 -traces trace/bzip2_10M.trace.gz trace/mcf_10M.trace.gz trace/libquantum_10M.trace.gz trace/xalancbmk_10M.trace.gz > multiple_core_result/harmony_four_pre.txt

compile_single_core:
	g++ -Wall --std=c++11 -o harmony_lru-config2 example/harmony.cc lib/config2.a

comile_multiple_core:
	g++ -Wall --std=c++11 -o harmony_lru-config4 example/harmony.cc lib/config4.a

test:
	g++ -Wall --std=c++11 -o lru-config3 example/lru-8MB.cc lib/config3.a
	./lru-config3 -warmup_instructions 1000000 -simulation_instructions 10000000 -traces trace/bzip2_10M.trace.gz trace/mcf_10M.trace.gz trace/libquantum_10M.trace.gz trace/xalancbmk_10M.trace.gz

clean:
	rm -f lru-config1 lru-config2 lru-config3 lru-config4
