#include<iostream>
#include<fstream>
#include<queue>
#include<vector>
#include<string>
#include<map>
#include<set>
#include<string>
#include<algorithm>
#include<list>

using namespace std;

ifstream ins_file;
ifstream data_in_file;
ifstream reg_file;
ofstream output;
ofstream data_out_file;
string s1, s2;

int data_stalling = 0;
int control_stalling = 0;

vector<bool> done(5,false);
int decoded_dest = -1;
int write_decoded_dest = -1;
char opcode;
vector<int> register_dependancies(16, 0);
int pc = 0;
int evaluated_value = -1;
int temp_reg_a = 1000, temp_reg_b = 1000;
bool branching = false;
bool exec_branching = false;
bool branching_done = false;
string param_ID = "";
int mem_decoded_dest = -1;
vector<string> instructions, data_cache;
vector<int> registers(16);
bool final_done = false;
pair<int, int> write_to_reg = {-1,-1};
int newpc = -1;

void write_back();
void mem_operation();
void execute();
void instruction_decode();
void instruction_fetch();

int mem_address_load = -1, mem_address_store = -1;

int total_instructions = 0;
int li_instructions = 0;
int logical_instructions = 0;
int shift_instructions = 0;
int arithmetic_instructions = 0;
int memory_instructions = 0;
int control_instructions = 0;
int halt_instructions = 0;
int total_cycles = -1;


int hex_to_num(char k){
    int ans = (int)k;
    if(ans >= 65) ans -= 87;
    else ans -= 48;
    return ans;
}

int signed_hex_to_num(char k){
    int ans = hex_to_num(k);
    if(ans > 7) ans -= 16;
    return ans;
}

string num_to_hex(int n){
    if(n < 0) n += 256;
    int a = n%16;
    if(a > 9) a += 87;
    else a += 48;
    int b = n/16;
    if(b > 9) b += 87;
    else b += 48;
    string ss = "";
    ss = ss + (char)b;
    ss = ss + (char)a;
    return ss;
}

void instruction_fetch(){
    if(!branching && !done[0]){
        param_ID = instructions[pc];
        pc = pc + 1;
    }
    else if(done[0]) done[1] = true;
    else if(branching_done){
        control_stalling += 2;
        param_ID = instructions[newpc];
        pc = newpc + 1;
        branching = false;
        exec_branching = false;
        branching_done = false;
        newpc = -1;
    }
    else return;
    
}

void instruction_decode(){
    if(done[1]) done[2] = true;
    if(param_ID == "") return;
    total_instructions++;
    if(param_ID[0] == '0') {opcode = '+'; arithmetic_instructions++;}
    if(param_ID[0] == '1') {opcode = '-'; arithmetic_instructions++;}
    if(param_ID[0] == '2') {opcode = '*'; arithmetic_instructions++;}
    if(param_ID[0] == '3') {opcode = '+'; arithmetic_instructions++;}
    if(param_ID[0] == '4') {opcode = '&'; logical_instructions++;}
    if(param_ID[0] == '5') {opcode = '|'; logical_instructions++;}
    if(param_ID[0] == '6') {opcode = '^'; logical_instructions++;}
    if(param_ID[0] == '7') {opcode = '~'; logical_instructions++;}
    if(param_ID[0] == '8') {opcode = '<'; shift_instructions++;}
    if(param_ID[0] == '9') {opcode = '>'; shift_instructions++;}
    if(param_ID[0] == 'a') {opcode = 'i'; li_instructions++;}
    if(param_ID[0] == 'b') {opcode = 'l'; memory_instructions++;}
    if(param_ID[0] == 'c') {opcode = 's'; memory_instructions++;}
    if(param_ID[0] == 'd') {opcode = 'j'; control_instructions++;}
    if(param_ID[0] == 'e') {opcode = 'b'; control_instructions++;}
    if(param_ID[0] == 'f') {opcode = 'h'; halt_instructions++;}
    if(opcode != 'j' && opcode != 'b' && opcode != 'h') decoded_dest = hex_to_num(param_ID[1]);
    if(opcode == '+' || opcode == '-' || opcode == '*' || opcode == '&' || opcode == '^' || opcode == '|' || opcode == '<' || opcode == '>'){
        if(param_ID[0] == '3'){
            if(register_dependancies[decoded_dest] != 0){
                total_cycles++;
                write_back();
                mem_operation();
                data_stalling++;
            }
            if(register_dependancies[decoded_dest] != 0){
                total_cycles++;
                write_back();
                data_stalling++;
            }
        }
        else{
            if(register_dependancies[hex_to_num(param_ID[2])] != 0 || register_dependancies[hex_to_num(param_ID[3])] != 0){
                total_cycles++;
                write_back();
                mem_operation();
                data_stalling++;
            }
            if(register_dependancies[hex_to_num(param_ID[2])] != 0 || register_dependancies[hex_to_num(param_ID[3])] != 0){
                total_cycles++;
                write_back();
                data_stalling++;
            }
        }


        register_dependancies[decoded_dest]++;
         // by now dependancy must be cleared

        if(param_ID[0] == '3'){
            temp_reg_a = registers[decoded_dest];
            temp_reg_b = 1;
        } 
        else{
        temp_reg_a = registers[hex_to_num(param_ID[2])];
        temp_reg_b = registers[hex_to_num(param_ID[3])];
        }
       
        param_ID = "";
        
    }
    else if(opcode == '~' || opcode == 'l' || opcode == 's'){
        if(opcode == '~' ){
            if(register_dependancies[hex_to_num(param_ID[2])] != 0){
            total_cycles++;
            write_back();
            mem_operation();
            data_stalling++;
            }
            if(register_dependancies[hex_to_num(param_ID[2])] != 0){
                total_cycles++;
                write_back();
                data_stalling++;
            }
            register_dependancies[decoded_dest]++;
        }
        if(opcode == 'l'){
            decoded_dest = hex_to_num(param_ID[1]);
            if(register_dependancies[hex_to_num(param_ID[2])] != 0){
            total_cycles++;
            write_back();
            mem_operation();
            data_stalling++;
            }
            if(register_dependancies[hex_to_num(param_ID[2])] != 0){
                total_cycles++;
                write_back();
                data_stalling++;
            }
            register_dependancies[decoded_dest]++;
        }
        else{
            if(register_dependancies[hex_to_num(param_ID[2])] != 0 || register_dependancies[hex_to_num(param_ID[1])] != 0){
                total_cycles++;
                write_back();
                mem_operation();
                data_stalling++;
            }
            if(register_dependancies[hex_to_num(param_ID[2])] != 0 || register_dependancies[hex_to_num(param_ID[1])] != 0){
                total_cycles++;
                write_back();
                data_stalling++;
            }
        }
        
        if(opcode == '~'){
            temp_reg_a = registers[hex_to_num(param_ID[2])];
            temp_reg_b = 1000;
        }
        else{
            temp_reg_a = registers[hex_to_num(param_ID[2])];
            temp_reg_b = signed_hex_to_num(param_ID[3]);
        }
       
        param_ID = "";
    }
    else if(opcode == 'i'){
        temp_reg_a = signed_hex_to_num(param_ID[2]) * 16 + hex_to_num(param_ID[3]); 
        temp_reg_b = 1000;
        param_ID = "";
        register_dependancies[decoded_dest]++;
    }
    else if(opcode == 'h') {done[0] = true; param_ID = "";}
    else{
        branching = true;
        if(opcode == 'j'){
            temp_reg_a = signed_hex_to_num(param_ID[1]) * 16 + hex_to_num(param_ID[2]); 
            temp_reg_b = 1000;   
        }
        else{
            if(register_dependancies[hex_to_num(param_ID[1])] != 0){
                total_cycles++;
                write_back();
                mem_operation();
                data_stalling++;
            }
            if(register_dependancies[hex_to_num(param_ID[1])] != 0){
                total_cycles++;
                write_back();
                data_stalling++;
            }
            temp_reg_a = registers[hex_to_num(param_ID[1])];
            temp_reg_b = signed_hex_to_num(param_ID[2]) * 16 + hex_to_num(param_ID[3]);
        }
        param_ID = "";
    }
}

void execute(){
    if(done[2]) done[3] = true; 
    if(branching) exec_branching = true;
    if(temp_reg_a == 1000) return;
    if(opcode == '+') evaluated_value = temp_reg_a + temp_reg_b;
    else if(opcode == '-') evaluated_value = temp_reg_a - temp_reg_b;
    else if(opcode == '*') evaluated_value = temp_reg_a * temp_reg_b;
    else if(opcode == '^') evaluated_value = temp_reg_a ^ temp_reg_b;
    else if(opcode == '&') evaluated_value = temp_reg_a & temp_reg_b;
    else if(opcode == '|') evaluated_value = temp_reg_a | temp_reg_b;
    else if(opcode == '~') evaluated_value = ~ temp_reg_a;
    else if(opcode == '<') evaluated_value = temp_reg_a << temp_reg_b;
    else if(opcode == '>') evaluated_value = temp_reg_a >> temp_reg_b;
    else if(opcode == 'l') evaluated_value = temp_reg_a + temp_reg_b;
    else if(opcode == 's') evaluated_value = temp_reg_a + temp_reg_b;
    else if(opcode == 'i') evaluated_value = temp_reg_a;
    else if(opcode == 'j') evaluated_value = pc + temp_reg_a;
    else if(opcode == 'b'){
        if(temp_reg_a == 0) evaluated_value = pc + temp_reg_b;
        else evaluated_value = pc;
    }
    else evaluated_value = -1;

    if(opcode != 'l' && opcode != 's' && opcode != 'j' && opcode != 'b'){
        write_decoded_dest = decoded_dest;
    }
    else if(opcode == 'l') {mem_address_load = evaluated_value; mem_decoded_dest = decoded_dest;}
    else if(opcode == 's') {mem_address_store = evaluated_value; mem_decoded_dest = decoded_dest;}

    decoded_dest = -1;
    temp_reg_a = 1000;
    temp_reg_b = 1000;
}

void mem_operation(){
    if(mem_address_load != -1) write_to_reg = {mem_decoded_dest, signed_hex_to_num(data_cache[mem_address_load][0]) * 16 + hex_to_num(data_cache[mem_address_load][1])};
    else if(mem_address_store != -1) data_cache[mem_address_store] = num_to_hex(registers[mem_decoded_dest]);
    else if(write_decoded_dest != -1) write_to_reg = {write_decoded_dest, evaluated_value};
    else if(exec_branching){
        branching_done = true;
        newpc = evaluated_value;
    }
    evaluated_value = -1;
    write_decoded_dest = -1;
    mem_decoded_dest = -1;
    mem_address_load = -1;
    mem_address_store = -1;
    if(done[3]) done[4] = true;
}

void write_back(){
    if(done[4]) final_done = true;
    if(write_to_reg.first != -1){
        registers[write_to_reg.first] = write_to_reg.second;
        register_dependancies[write_to_reg.first]--;
    }
    write_to_reg = {-1,-1};
}


int main(){
    string s;
    ins_file.open("input/ICache.txt");
    if (!ins_file) cerr << "Could not open the file3!" << endl;
    while(!ins_file.eof()){
        getline(ins_file, s1, '\n');
        getline(ins_file, s2, '\n');
        s = s1 + s2;
        instructions.push_back(s);
    }
    ins_file.close();
    reg_file.open("input/RF.txt");
    if (!reg_file) cerr << "Could not open the file4!" << endl;
    for(int i = 0; i < 16; i++){
        getline(reg_file, s1, '\n');
        registers[i] = signed_hex_to_num(s1[0]) * 16 + hex_to_num(s1[1]);
    }
    reg_file.close();
    data_in_file.open("input/DCache.txt");
    if (!data_in_file) cerr << "Could not open the file5!" << endl;
    while(!data_in_file.eof()){
        getline(data_in_file, s1, '\n');
        data_cache.push_back(s1);
    }
    data_in_file.close();
    while(!final_done){
        total_cycles++;
        write_back();
        if(final_done) break;
        mem_operation();
        execute();
        instruction_decode();
        instruction_fetch();
    }
    float cycle_per_in = (float)total_cycles/total_instructions;
    output.open("output/Output.txt");
    if (!output) cerr << "Could not open the file1!" << endl;
    output << "Total number of instructions executed: " << total_instructions << '\n';
    output << "Number of instructions in each class\n";
    output << "Arithmetic instructions: " << arithmetic_instructions << '\n';
    output << "Logical instructions: " << logical_instructions << '\n';
    output << "Shift instructions: " << shift_instructions << '\n';
    output << "Memory instructions: " << memory_instructions << '\n';
    output << "Load immediate instructions: " << li_instructions << '\n';
    output << "Control instructions: " << control_instructions << '\n';
    output << "Halt instructions: " << halt_instructions << '\n';
    output << "Cycles per instruction: " << cycle_per_in << '\n';
    output << "Total number of stalls: " << data_stalling + control_stalling << '\n';
    output << "Data stalls (RAW): " << data_stalling << '\n';
    output << "Control stalls: " << control_stalling << '\n';
    output.close();
    data_out_file.open("output/DCache.txt");
    if (!data_out_file) cerr << "Could not open the file2!" << endl;
    for(int i = 0; i < 256; i++){
        data_out_file << data_cache[i] << '\n';
    }
    data_out_file.close();
    return 0;
}