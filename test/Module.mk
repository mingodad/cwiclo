################ Source files ##########################################

test/SRCS	:= $(wildcard test/*.cc)
test/TSRCS	:= $(wildcard test/?????.cc)
test/TESTS	:= $(addprefix $O,$(test/TSRCS:.cc=))
test/OBJS	:= $(addprefix $O,$(test/SRCS:.cc=.o))
test/DEPS	:= ${test/OBJS:.o=.d}
test/OUTS	:= ${test/TESTS:=.out}

################ Compilation ###########################################

.PHONY:	test/all test/run test/clean test/check

test/all:	${test/TESTS}

# The correct output of a test is stored in testXX.std
# When the test runs, its output is compared to .std
#
check:		test/check
test/check:	${test/TESTS}
	@for i in ${test/TESTS}; do \
	    TEST="test/$$(basename $$i)";\
	    echo "Running $$TEST";\
	    PATH="$Otest" $$i < $$TEST.cc &> $$i.out;\
	    diff $$TEST.std $$i.out && rm -f $$i.out;\
	done

$Otest/tlibf:	$Otest/tlibf.o ${LIBA}
	@echo "Linking $@ ..."
	@${CC} ${LDFLAGS} -o $@ $^

$Otest/fwork:	$Otest/fwork.o $Otest/ping.o ${LIBA}
	@echo "Linking $@ ..."
	@${CC} ${LDFLAGS} -o $@ $^

$Otest/ipcom:	$Otest/ipcom.o $Otest/ping.o ${LIBA} | $Otest/ipcomsrv
	@echo "Linking $@ ..."
	@${CC} ${LDFLAGS} -o $@ $^

$Otest/ipcomsrv:	$Otest/ipcomsrv.o $Otest/ping.o ${LIBA}
	@echo "Linking $@ ..."
	@${CC} ${LDFLAGS} -o $@ $^

################ Maintenance ###########################################

clean:	test/clean
test/clean:
	@if [ -d $Otest ]; then\
	    rm -f ${test/TESTS} $Otest/ipcomsrv ${test/OBJS} ${test/DEPS} ${test/OUTS} $Otest/.d;\
	    rmdir ${BUILDDIR}/test;\
	fi

${test/OBJS}: Makefile test/Module.mk ${CONFS} $Otest/.d

-include ${test/DEPS}
