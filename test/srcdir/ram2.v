`define WORD_SIZE 8

module ternary_content_addressable_memory2 #(
    parameter word_size = `WORD_SIZE,
    parameter address_size = 4
) (
    matched,
    word,
    mask,
    address,
    write,
    clock,
    reset
);

    output reg [1 << address_size - 1:0] matched;
    input wire [word_size - 1:0] word;
    input wire [word_size - 1:0] mask;
    input wire [address_size - 1:0] address;
    input wire write;
    input wire clock;
    input wire reset;

    reg [word_size - 1:0] memory [(1 << address_size) - 1:0];

    reg [1 << address_size - 1:0] i;
    reg [word_size - 1:0] j;
    reg flag;

    always @(posedge clock)
    begin
        if (write)
            memory[address] = word;
        else
        begin
            for (i = 0; i < 1 << address_size; i++)
            begin
            flag = 1;
                for (j = 0; j < word_size; j++)
                    if (!mask[j] & memory[i][j] != word[j])
                        flag = 0;
                matched[i] = flag;
            end
        end
    end

    always @(posedge reset)
    begin
        for (i = 0; i < 1 << address_size; i++)
            memory[i] = 0;
        matched = 0;
    end

endmodule