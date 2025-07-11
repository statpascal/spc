unit system;

(* Declaration of runtime routines implemented as C functions *)

interface

const
    pi = 3.141592653589793;
    MaxInt = $7fffffff;

type
    shortint = int8;
    smallint = int16;
    longint = int32;
    integer = int32;
    comp = int64;
    
    byte = uint8;
    word = uint16;
    cardinal = uint32;
    longword = uint32;
    
    double = real;
    PChar = ^char;
    rt_string = PChar;
    
var
    input, output: text;

(* Standard functions *)

function sqrt (x: double): double; external;
function sin (x: double): double; external;
function arcsin (x: double): double; external name 'asin';
function cos (x: double): double; external;
function arccos (x: double): double; external name 'acos';
function tan (x: double): double; external;
function arctan (x: double): double; external name 'atan';
function cosh (x: double): double; external;
function sinh (x: double): double; external;
function tanh (x: double): double; external;
function log (x: double): double; external name 'log10';
function ln (x: double): double; external name 'log';
function exp (x: double): double; external;
function floor (x: double): double; external;
function ceil (x: double): double; external;
function frac (x: double): double; external name 'rt_dbl_frac';
function power (x, y: double): double; external name 'pow';

function abs (n: int64): int64; external name 'rt_int_abs';
function abs (x: double): double; external name 'rt_dbl_abs';

function sign (n: int64): int64; external name 'rt_int_sign';
function sign (x: double): int64; external name 'rt_dbl_sign';

function sqr (n: int64): int64; external name 'rt_int_sqr';
function sqr (x: double): double; external name 'rt_dbl_sqr';

function round (x: double): int64; external name 'rt_dbl_round';
function trunc (x: double): int64; external name 'rt_dbl_trunc';
function upcase (ch: char): char; external name 'rt_char_upcase';
function upcase (s: string): string; external name 'rt_str_upcase';

procedure randomize; external name 'rt_randomize';
procedure randomize (seed: int64); external name 'rt_randomize_seed';
function random: double; external name 'rt_dbl_random';
function random (n: int64): int64; external name 'rt_int_random';

procedure getmem (var p: pointer; n: int64); external name 'rt_dyn_alloc';
procedure freemem (p: pointer; n: int64); external name 'rt_dyn_free';

procedure fillchar (var x; count: int64; value: uint8); external name 'rt_fillchar';
procedure fillchar (var x; count: int64; value: boolean); external name 'rt_fillchar';
procedure fillchar (var x; count: int64; value: char); external name 'rt_fillchar';
procedure move (var source, dest; count: int64); external name 'rt_move';
function comparebyte (var buf1, buf2; count: int64): int64; external name 'rt_compare_byte';

function __val_int (s: rt_string; var code: uint16): int64; external name 'rt_val_int';
function __val_dbl (s: rt_string; var code: uint16): real; external name 'rt_val_dbl';

function min (a, b: int64): int64; external name 'rt_min_int';
function min (a, b: real): real; external name 'rt_min_dbl';
function max (a, b: int64): int64; external name 'rt_max_int';
function max (a, b: real): real; external name 'rt_max_dbl';

(* Vector handling *)

type 
    realvector = vector of real;
    int64vector = vector of int64;
    boolvector = vector of boolean;
    charvector = vector of char;
    stringvector = vector of string;

function intvec (a, b: int64): int64vector; external name 'rt_intvec';
function realvec (a, b, step: real): realvector; external name 'rt_realvec';
function __make_vec_int (size, val: int64): __generic_vector; external name 'rt_makevec_int';
function __make_vec_dbl (size: int64; val: real): __generic_vector; external name 'rt_makevec_dbl';
function __make_vec_str (anyManagerIndex: int64; runtimeData: pointer; s: string): __generic_vector; external name 'rt_makevec_str';
function __make_vec_vec (anyManagerIndex: int64; runtimeData: pointer; a: __generic_vector): __generic_vector; external name 'rt_makevec_vec';
function __combine_vec_4 (a, b, c, d: __generic_vector): __generic_vector; external name 'rt_combinevec_4';
function __combine_vec_3 (a, b, c: __generic_vector): __generic_vector; external name 'rt_combinevec_3';
function __combine_vec_2 (a, b: __generic_vector): __generic_vector; external name 'rt_combinevec_2';
function __size_vec (a: __generic_vector): int64; external name 'rt_sizevec';
procedure __resize_vec (anyManagerIndex: int64; runtimeData: pointer; elsize: int64; var a: __generic_vector; size: int64); external name 'rt_resizevec';
function __rev_vec (a: __generic_vector): __generic_vector; external name 'rt_revvec';

function randomperm (n: int64): int64vector; external name 'rt_vint_randomperm';
function randomdata (n: int64): realvector; external name 'rt_vdbl_random';
function randomdata (m, n: int64): int64vector; external name 'rt_vint_random';


function sqr (x: realvector): realvector; external name 'rt_vdbl_sqr';
function sqrt (x: realvector): realvector; external name 'rt_vdbl_sqrt';
function sin (x: realvector): realvector; external name 'rt_vdbl_sin';
function cos (x: realvector): realvector; external name 'rt_vdbl_cos';
function log (x: realvector): realvector; external name 'rt_vdbl_log';
function power (x: realvector; y: double): realvector; external name 'rt_vdbl_pow';

function __vec_add (a, b: __generic_vector; tca, tcb: int64): __generic_vector; external name 'rt_vec_add';
function __vec_sub (a, b: __generic_vector; tca, tcb: int64): __generic_vector; external name 'rt_vec_sub';
function __vec_or (a, b: __generic_vector; tca, tcb: int64): __generic_vector; external name 'rt_vec_or';
function __vec_xor (a, b: __generic_vector; tca, tcb: int64): __generic_vector; external name 'rt_vec_xor';
function __vec_mul (a, b: __generic_vector; tca, tcb: int64): __generic_vector; external name 'rt_vec_mul';
function __vec_div (a, b: __generic_vector; tca, tcb: int64): __generic_vector; external name 'rt_vec_div';
function __vec_mod (a, b: __generic_vector; tca, tcb: int64): __generic_vector; external name 'rt_vec_mod';
function __vec_and (a, b: __generic_vector; tca, tcb: int64): __generic_vector; external name 'rt_vec_and';
function __vec_shl (a, b: __generic_vector; tca, tcb: int64): __generic_vector; external name 'rt_vec_shl';
function __vec_shr (a, b: __generic_vector; tca, tcb: int64): __generic_vector; external name 'rt_vec_shr';




function __vec_equal (a, b: __generic_vector; tca, tcb: int64): __generic_vector; external name 'rt_vec_equal';
function __vec_not_equal (a, b: __generic_vector; tca, tcb: int64): __generic_vector; external name 'rt_vec_not_equal';
function __vec_less_equal (a, b: __generic_vector; tca, tcb: int64): __generic_vector; external name 'rt_vec_less_equal';
function __vec_greater_equal (a, b: __generic_vector; tca, tcb: int64): __generic_vector; external name 'rt_vec_greater_equal';
function __vec_less (a, b: __generic_vector; tca, tcb: int64): __generic_vector; external name 'rt_vec_less';
function __vec_greater (a, b: __generic_vector; tca, tcb: int64): __generic_vector; external name 'rt_vec_greater';

function __vec_conv (a: __generic_vector; tcs, tcd: int64): __generic_vector; external name 'rt_vec_conv';

function __vec_index_int (a: __generic_vector; index: int64): pointer; external name 'rt_vec_index_int';
function __vec_index_vint (a, index: __generic_vector): __generic_vector; external name 'rt_vec_index_vint';
function __vec_index_vbool (a, index: __generic_vector): __generic_vector; external name 'rt_vec_index_vbool';

function sort (x: int64vector): int64vector; external name 'rt_vint_sort';
function sort (x: realvector): realvector; external name 'rt_vdbl_sort';

function sum (x: int64vector): int64; external name 'rt_vint_sum';
function sum (x: realvector): double; external name 'rt_vdbl_sum';
function count (x: boolvector): int64; external name 'rt_vbool_count';

function cumsum (x: int64vector): int64vector; external name 'rt_vint_cumsum';
function cumsum (x: realvector): realvector; external name 'rt_vdbl_cumsum';


(* Text files *)

procedure append (var f: text); 

function eoln: boolean;
function eoln (var f: text): boolean; 

function fileexists (filename: rt_string): boolean; external name 'rt_file_exists';
function deletefile (filename: rt_string): boolean; external name 'rt_file_delete';
function extractFilePath (filename: string): string; external name 'rt_file_path';
function extractFileName (filename: string): string; external name 'rt_file_name';

(* Internal usage *)

var
    __GlobalRuntimeData: pointer;

procedure __assign (var f; filename: string);
procedure __reset_text (var f: text; runtimeData: pointer); external name 'rt_text_reset';
procedure __reset_bin (var f; blocksize: int64; runtimeData: pointer); external name 'rt_bin_reset';
procedure __rewrite_text (var f: text; runtimeData: pointer); external name 'rt_text_rewrite';
procedure __rewrite_bin (var f; blocksize: int64; runtimeData: pointer); external name 'rt_bin_rewrite';
procedure __text_append (var f: text; runtimeData: pointer); external name 'rt_text_append';

procedure __seek_bin (var f; pos: int64; runtimeData: pointer); external name 'rt_bin_seek';
function __filesize_bin (var f; runtimeData: pointer): int64; external name 'rt_bin_filesize';
function __filepos_bin (var f; runtimeData: pointer): int64; external name 'rt_bin_filepos';
procedure __flush (var f; runtimeData: pointer); external name 'rt_flush';
procedure __erase (var f); external name 'rt_erase';
procedure __close (var f; runtimeData: pointer); external name 'rt_close';

function __eof_input: boolean;
function __eof (var f; runtimeData: pointer): boolean; external name 'rt_eof';
function __text_eoln (var f: text; runtimeData: pointer): boolean; external name 'rt_text_eoln';

procedure __write_bin_typed (var f, buf);
procedure __write_bin_ign (var f, buf; size: int64);
procedure __write_bin (var f, buf; size: int64; var blockswritten: int64; runtimeData: pointer); external name 'rt_bin_write';
procedure __read_bin_typed (var f, buf);
procedure __read_bin_ign (var f, buf; size: int64);
procedure __read_bin (var f, buf; size: int64; var blocksread: int64; runtimeData: pointer); external name 'rt_bin_read';

procedure __write_lf (var f: text; runtimeData: pointer); external name 'rt_write_lf';
procedure __read_lf (var f: text; runtimeData: pointer); external name 'rt_read_lf';

procedure __write_int64 (var f: text; n, length, precision: int64; runtimeData: pointer); external name 'rt_write_int';
procedure __write_char (var f: text; ch: char; length, precision: int64; runtimeData: pointer); external name 'rt_write_char';
procedure __write_string (var f: text; s: ^char; length, precision: int64; runtimeData: pointer); external name 'rt_write_string';
procedure __write_boolean (var f: text; b: boolean; length, precision: int64; runtimeData: pointer); external name 'rt_write_bool';
procedure __write_dbl (var f: text; a: real; length, precision: int64; runtimeData: pointer); external name 'rt_write_dbl';

function __read_int64 (var f: text; runtimeData: pointer): int64; external name 'rt_read_int';
function __read_char (var f: text; runtimeData: pointer): char; external name 'rt_read_char';
function __read_string (var f: text; runtimeData: pointer): string; external name 'rt_read_string';
function __read_dbl (var f: text; runtimeData: pointer): real; external name 'rt_read_dbl';

procedure __write_vint64 (var f: text; n: int64vector; length, precision: int64; runtimeData: pointer); external name 'rt_write_vint';
procedure __write_vchar (var f: text; ch: charvector; length, precision: int64); external name 'rt_write_vchar';
procedure __write_vstring (var f: text; s: stringvector; length, precision: int64); external name 'rt_write_vstring';
procedure __write_vboolean (var f: text; b: boolvector; length, precision: int64; runtimeData: pointer); external name 'rt_write_vbool';
procedure __write_vdbl (var f: text; a: realvector; length, precision: int64; runtimeData: pointer); external name 'rt_write_vdbl';

procedure __str_int64 (n, length, precision: int64; var s: string); external name 'rt_str_int';
procedure __str_dbl (a: real; length, precision: int64; var s: string); external name 'rt_str_dbl';

type
    __generic_set_type = set of 0..255;
    
function __set_const (index: int64; runtimeData: pointer): __generic_set_type; external name 'rt_get_set_const';
function __set_add_val (val: int64; var s: __generic_set_type): __generic_set_type; external name 'rt_set_add_val';
function __set_add_range (minval, maxval: int64; var s: __generic_set_type): __generic_set_type; external name 'rt_set_add_range';

function __in_set (v: int64; var s: __generic_set_type): boolean; external name 'rt_in_set';
function __set_union (var s, t: __generic_set_type): __generic_set_type; external name 'rt_set_union';
function __set_intersection (var s, t: __generic_set_type): __generic_set_type; external name 'rt_set_intersection';
function __set_diff (var s, t: __generic_set_type): __generic_set_type; external name 'rt_set_diff';

function __set_equal (var s, t: __generic_set_type): boolean; external name 'rt_set_equal';
function __set_not_equal (var s, t: __generic_set_type): boolean; external name 'rt_set_not_equal';
function __set_sub (var s, t: __generic_set_type): boolean; external name 'rt_set_sub';
function __set_super (var s, t: __generic_set_type): boolean; external name 'rt_set_super';
function __set_sub_not_equal (var s, t: __generic_set_type): boolean; external name 'rt_set_sub_not_equal';
function __set_super_not_equal (var s, t: __generic_set_type): boolean; external name 'rt_set_super_not_equal';

function __str_equal (s, t: string): boolean; external name 'rt_str_equal';
function __str_not_equal (s, t: string): boolean; external name 'rt_str_not_equal';
function __str_less_equal (s, t: string): boolean; external name 'rt_str_less_equal';
function __str_greater_equal (s, t: string): boolean; external name 'rt_str_greater_equal';
function __str_less (s, t: string): boolean; external name 'rt_str_less';
function __str_greater (s, t: string): boolean; external name 'rt_str_greater';

function __str_make (index: int64; runtimeData: pointer): string; external  name 'rt_str_make';
function __str_concat (a, b: string): string; external  name 'rt_str_concat';
function __str_char (a: char): string; external name 'rt_str_char';
function __str_index (s: string; index: int64): pointer; external  name 'rt_str_index';

procedure __copy_mem (dst, src: pointer; size, anyManagerIndex: int64; deleteDst: boolean; runtimeData: pointer); external name 'rt_copy_mem';
procedure __memcpy (dst, src: pointer; size: int64); external name 'memcpy';
function __alloc_mem (count, size, anyManagerIndex: int64; runtimeData: pointer): pointer; external name 'rt_alloc_mem';
procedure __free_mem (p, runtimeData: pointer); external name 'rt_free_mem';
procedure __init_mem (p: pointer; anyManagerIndex: int64; runtimeData: pointer); external name 'rt_init_mem';
procedure __destroy_mem (p: pointer; anyManagerIndex: int64; runtimeData: pointer); external name 'rt_destroy_mem';
procedure __halt (n: int64; runtimeData: pointer); external name 'rt_halt';

(* String handling *)

procedure insert (s: string; var t: string; pos: int64); external name 'rt_str_insert';
function length (s: string): int64;  external name 'rt_str_length';
function pos (needle, haystack: string): int64; external name 'rt_str_pos';
function copy (s: rt_string; pos, length: int64): string; external  name 'rt_str_copy';
procedure delete (var s: string; pos, length: int64); external name 'rt_str_delete';
function stringofchar (ch: char; count: int64): string; external  name 'rt_str_of_char';

(* Command line parameters *)

function ParamCount: int64;
function ParamStr (n: int64): string;

implementation

function __argc (runtimeData: pointer): int64; external name 'rt_argc';
procedure __argv (n: int64; runtimeData: pointer; var s: string); external name 'rt_argv';

procedure __assign (var f; filename: string);
    begin
        text (f).fn := filename
    end;

procedure append (var f: text);
    begin
        __text_append (f, __GlobalRuntimeData)
    end;

function __eof_input: boolean;
    begin
        __eof_input := __eof (input, __GlobalRuntimeData)
    end;
    
function eoln (var f: text): boolean;
    begin
        eoln := __text_eoln (f, __GlobalRuntimeData)
    end;

function eoln: boolean;
    begin
        eoln := __text_eoln (input, __GlobalRuntimeData)
    end;

procedure __write_bin_typed (var f, buf);
    var written: int64;
    begin
        __write_bin (f, buf, 1, written, __GlobalRuntimeData);
    end;

procedure __write_bin_ign (var f, buf; size: int64);
    var written: int64;
    begin
        __write_bin (f, buf, size, written, __GlobalRuntimeData)
    end;
    
procedure __read_bin_typed (var f, buf);
    var read: int64;
    begin
        __read_bin (f, buf, 1, read, __GlobalRuntimeData)
    end;

procedure __read_bin_ign (var f, buf; size: int64);
    var read: int64;
    begin
        __read_bin (f, buf, size, read, __GlobalRuntimeData)
    end;

function ParamCount: int64;
    begin
        ParamCount := __argc (__GlobalRuntimeData)
    end;

function ParamStr (n: int64): string;
    var
        s: string;
    begin
        __argv (n, __GlobalRuntimeData, s);
        ParamStr := s
    end;

begin
    assign (input, '');
    reset (input);
    assign (output, '');
    rewrite (output)
end.
