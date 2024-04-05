module tristate(
	inout signal,
	input select,
	input to_signal,
	output from_signal
);
IOBUF sd(.T(select),.IO(signal),.I(to_signal),.O(from_signal) );
endmodule
