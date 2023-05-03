module testbench;

    localparam word_size = `WORD_SIZE;
    localparam address_size = 4;

    wire [1 << address_size - 1:0] matched;
    reg [word_size - 1:0] word;
    reg [word_size - 1:0] mask;
    reg [address_size - 1:0] address;
    reg write;
    reg clock;
    reg reset;

    ternary_content_addressable_memory #(
        .word_size(word_size),
        .address_size(address_size)
    ) tcam (
        .matched(matched),
        .word(word),
        .mask(mask),
        .address(address),
        .write(write),
        .clock(clock),
        .reset(reset)
    );

    initial clock = 0;

    always #5
        clock = ~clock;

    initial
    begin
        $dumpfile("DSD_99105504_Final_Q03.vcd");
        $dumpvars(0, testbench);
    end

    initial
    begin
        write = 0;
        #1 reset = 1;
        #1 reset = 0;
        write = 1;
        word = 8'b1001_0111;
        address = 4'b0001;
        #10
        word = 8'b1011_0111;
        address = 4'b0100;
        #10
        mask = 8'b0010_0000;
        write = 0;
        word = 8'b1001_0111;
        #10
        $display("matched = %b", matched);
        mask = 0;
        word = 8'b1011_0111;
        #10
        $display("matched = %b", matched);
        $finish();
    end

endmodule
