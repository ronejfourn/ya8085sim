#include "tokenizer.h"
#include "memory.h"
#include "tables.h"
#include "iset.h"
#include <stdio.h>
#include <stdlib.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

#define HEX2NORM(hx) ((((hx) >> 16) & 0xff) / 255.0), ((((hx) >> 8) & 0xff) / 255.0), ((((hx) >> 0) & 0xff) / 255.0)

#if defined(_WIN32) || defined(_WIN64)
#define _CRT_SECURE_NO_WARNINGS 1
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef IN
#undef OUT
#endif

enum {
	REG_A, REG_F, REG_B, REG_C,
	REG_D, REG_E, REG_H, REG_L,
	REG_COUNT,
};

enum {
	FLAG_NONE = 0,
	FLAG_S    = 1 << 7,
	FLAG_Z    = 1 << 6,
	FLAG_AC   = 1 << 4,
	FLAG_P    = 1 << 3,
	FLAG_CY   = 1,
};

static uint8_t registers[REG_COUNT];
static uint8_t IO[0x100];
extern uint8_t memory[0x10000];

#define UPPER_BYTE_B registers[REG_B]
#define UPPER_BYTE_D registers[REG_D]
#define UPPER_BYTE_H registers[REG_H]
#define UPPER_BYTE_SP *SPH
#define UPPER_BYTE_PSW registers[REG_A]

#define LOWER_BYTE_B registers[REG_C]
#define LOWER_BYTE_D registers[REG_E]
#define LOWER_BYTE_H registers[REG_L]
#define LOWER_BYTE_SP *SPL
#define LOWER_BYTE_PSW registers[REG_F]
#define REG_PAIR(x) ((uint16_t)UPPER_BYTE_ ##x << 8 | (uint16_t)LOWER_BYTE_ ##x)

#define _REG__A registers[REG_A]
#define _REG__F registers[REG_F]
#define _REG__B registers[REG_B]
#define _REG__C registers[REG_C]
#define _REG__D registers[REG_D]
#define _REG__E registers[REG_E]
#define _REG__H registers[REG_H]
#define _REG__L registers[REG_L]
#define _REG__M memory[REG_PAIR(H)]
#define REGISTER(x) _REG__ ##x

#define SET(f, b) ((f) |= (b))
#define RESET(f, b) ((f) &= ~(b))
#define TOGGLE(f, b) ((f) ^= (b))

static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

static void glfw_framebuffer_size_callback(GLFWwindow *window, int w, int h) {
	glViewport(0, 0, w, h);
}

uint8_t ALU(uint8_t op1, uint8_t op2, char op, uint8_t cc) {
	uint8_t res;
	switch (op) {
		case '-':
			op2 = ~op2 + 1;
		case '+':
			res = op1 + op2; break;
		case '&':
			res = op1 & op2; break;
		case '|':
			res = op1 | op2; break;
		case '^':
			res = op1 ^ op2; break;
	}

	res == 0 ?
		SET(REGISTER(F), FLAG_Z):
		RESET(REGISTER(F), FLAG_Z);

	(res & 0x80) == 0 ?
		RESET(REGISTER(F), FLAG_S):
		SET(REGISTER(F), FLAG_S);

	if (cc) {
		if (op == '-')
			(op1 + op2 > 0xff) ?
				RESET(REGISTER(F), FLAG_CY):
				SET(REGISTER(F), FLAG_CY);
		else if (op == '+')
			(op1 + op2 > 0xff) ?
				SET(REGISTER(F), FLAG_CY):
				RESET(REGISTER(F), FLAG_CY);
	}

	uint8_t cnt = 0;
	for (uint8_t cpy = res; cpy != 0; cpy >>= 1)
		cnt += cpy & 1;
	(cnt & 1) ?
		RESET(REGISTER(F), FLAG_P):
		SET(REGISTER(F), FLAG_P);

	if (op == '-')
		((op1 & 0xf) + (op2 & 0xf) > 0xf) ?
			SET(REGISTER(F), FLAG_AC):
			RESET(REGISTER(F), FLAG_AC);
	else if (op == '+')
		((op1 & 0xf) + (op2 & 0xf) > 0xf) ?
			RESET(REGISTER(F), FLAG_AC):
			SET(REGISTER(F), FLAG_AC);

	return res;
}

char *read_entire_file(char *fname) {
	FILE *inf = fopen(fname, "rb");
	if (!inf) {
		fprintf(stderr, "could not open file %s\n", fname);
		return NULL;
	}

	fseek(inf, 0, SEEK_END);
	int fs = ftell(inf);
	rewind(inf);

	char *buf = (char *)malloc(fs + 1);
	if (!buf) {
		fprintf(stderr, "out of memory\n");
		fclose(inf);
		return NULL;
	}
	fread(buf, 1, fs, inf);
	fclose(inf);
	buf[fs] = 0;
	return buf;
}

int main(int argc, char **argv) {
	if (argc != 2) {
		fprintf(stdout, "Usage: ./ya8085sim [filename]\n");
		return 0;
	}

	char *buf = read_entire_file(argv[1]);
	if (!buf) return 1;

	instruction_table_init();
	partial_instruction_table_init();
	symbol_table_init();
	symbol_queue_init();

	set_loadat(0x4200);

	// Tokenize

	tokenizer tk;
	tk.row = 0;
	tk.col = 0;
	tk.data = buf;

	int token_count = 0;
	int instruction_count = 0;
	int prev = -1;
	int no_errors = 1;
	while (*tk.data) {
		token t = tokenizer_get_next(&tk);
		if (t.type == TOKEN_ERR) {
			fprintf(stderr, "%s\n", t.err);
			no_errors = 0;
		}
		if (t.type == TOKEN_SYM && t.row != prev) {
			instruction_count ++;
			prev = t.row;
		}
		token_count ++;
	}

	tk.row = 0;
	tk.col = 0;
	tk.data = buf;
	int token_index = 0;
	token *token_table = (token *)malloc(sizeof(token) * token_count);
	while (*tk.data)
		token_table[token_index++] = tokenizer_get_next(&tk);

	struct immap {
		int mp, ti, bc;
	} *ins_to_mem_map = (struct immap*)malloc(sizeof(*ins_to_mem_map) * instruction_count);

	// Parse tokens

	int instruction_index = 0;
	token_index = 0;
	while (token_index < token_count && no_errors) {
		if (token_table[token_index].type == TOKEN_EOI || token_table[token_index].type == TOKEN_CMT) {
			token_index ++;
			continue;
		}

		if (token_table[token_index].type != TOKEN_SYM) {
			token bad_token = token_table[token_index];
			char *token_str = token_val_str(bad_token);
			fprintf(stderr, "at [%i:%i] expected symbol, got '%s' (%s)\n",
					bad_token.row, bad_token.col,
					token_str, token_name(bad_token.type));
			free(token_str);
			no_errors = 0;
			break;
		}

		ins_to_mem_map[instruction_index].ti = token_index;
		ins_to_mem_map[instruction_index].mp = get_load_pos();

		if (token_table[token_index + 1].type == TOKEN_COL) {
			token label = token_table[token_index];
			char *error = symbol_table_add(ls_from_parts(label.sym, label.len), get_load_pos());
			if (error) {
				fprintf(stderr, "at [%i:%i] %s\n",
						label.row, label.col, error);
				free(error);
				no_errors = 0;
				break;
			}
			token_index ++;
			if (token_table[token_index + 1].type != TOKEN_SYM) {
				token bad_token = token_table[token_index];
				char *token_str = token_val_str(bad_token);
				fprintf(stderr, "at [%i:%i] expected instruction, got '%s' (%s)\n",
						bad_token.row, bad_token.col,
						token_str, token_name(bad_token.type));
				free(token_str);
				no_errors = 0;
				break;
			}
			token_index ++;
		}

		token instruction = token_table[token_index];
		char *error = load_instruction(ls_from_parts(instruction.sym, instruction.len), token_table, &token_index);
		if (error) {
			fprintf(stderr, "at [%i:%i] '%s'\n", token_table[token_index].row, token_table[token_index].col, error);
			free(error);
			no_errors = 0;
			break;
		}

		ins_to_mem_map[instruction_index].bc = get_load_pos() - ins_to_mem_map[instruction_index].mp;
		instruction_index ++;
	}

	// Resolve forward references

	if (no_errors) {
		char *error = symbol_resolve();
		if (error) {
			no_errors = 0;
			fprintf(stderr, "%s\n", error);
			free(error);
		}
	}


    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if defined(__APPLE__)
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Dear ImGui GLFW+OpenGL3 example", NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, glfw_framebuffer_size_callback);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Our state
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	char program_should_run = 0;
	uint16_t PC = get_loadat();
	uint16_t SP = 0xFFFF;
	uint16_t WZ = 0x0000;
	uint8_t *SPH = (uint8_t *)&SP;
	uint8_t *SPL = (uint8_t *)&SP + 1;

	memory[0x2040] = 5;
	uint8_t numbers[] = { 9, 3, 2, 4, 1 };
	memcpy(memory + 0x2041, numbers, sizeof(numbers));

	static ImGuiTableFlags flags = ImGuiTableFlags_RowBg  | ImGuiTableFlags_Resizable;
	uint16_t memstart = 0;
	uint8_t  iostart  = 0;
	uint16_t stkstart = SP;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

		ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);

		ImGui::Begin("Listing");
		instruction_index = 0;
        if (ImGui::BeginTable("##memory", 3, flags)) {
			ImGui::TableSetupColumn("Address (Hex)");
			ImGui::TableSetupColumn("Opcode");
			ImGui::TableSetupColumn("Assembly");
			ImGui::TableHeadersRow();

			for (token_index = 0; token_index < token_count && no_errors; token_index ++) {
				token t = token_table[token_index];
				ImGui::TableNextRow();

				if (t.type == TOKEN_SYM || t.type == TOKEN_INS) {
					struct immap a = ins_to_mem_map[instruction_index];
					ImGui::TableSetColumnIndex(0);
					ImGui::TextColored(ImVec4(HEX2NORM(0x98BB6C), 1.0f), "%.4XH", a.mp);

					ImGui::TableSetColumnIndex(1);
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(HEX2NORM(0x938AA9), 1.0f));
					switch (a.bc) {
						case 1: ImGui::Text("%.2X", memory[a.mp]); break;
						case 2: ImGui::Text("%.2X %.2X", memory[a.mp], memory[a.mp + 1]); break;
						case 3: ImGui::Text("%.2X %.2X %.2X", memory[a.mp], memory[a.mp + 1], memory[a.mp + 2]); break;
					}
					ImGui::PopStyleColor();

					prev = t.row;
					ImGui::TableSetColumnIndex(2);
					while (t.row == prev && token_index < token_count) {
						switch (t.type) {
							case TOKEN_COM:
								ImGui::Text(","); break;
							case TOKEN_COL:
								ImGui::Text(":"); break;
							case TOKEN_SYM:
								ImGui::TextColored(ImVec4(HEX2NORM(0xE6C384), 1.0f), "%.*s", t.len, t.sym); break;
							case TOKEN_INS:
								ImGui::TextColored(ImVec4(HEX2NORM(0x957FB8), 1.0f), "%.*s", t.len, t.sym); break;
							case TOKEN_REG:
							case TOKEN_REP:
								ImGui::TextColored(ImVec4(HEX2NORM(0x7AA89F), 1.0f), "%.*s", t.len, t.sym); break;
							case TOKEN_WRD:
								ImGui::TextColored(ImVec4(HEX2NORM(0xD27E99), 1.0f), "%.2XH ", t.num); break;
							case TOKEN_DWD:
								ImGui::TextColored(ImVec4(HEX2NORM(0xD27E99), 1.0f), "%.4XH ", t.num); break;
							case TOKEN_CMT:
								ImGui::TextColored(ImVec4(HEX2NORM(0x727169), 1.0f), ";%.*s", t.len, t.cmt); break;
							default: break;
						}
						ImGui::SameLine();
						t = token_table[++token_index];
					}

					token_index --;
					instruction_index ++;
				} else if (t.type == TOKEN_CMT){
					ImGui::TableSetColumnIndex(2);
					ImGui::TextColored(ImVec4(HEX2NORM(0x727169), 1.0f), ";%.*s", t.len, t.cmt);
				}
			}
            ImGui::EndTable();
        }

		ImGui::End();
		ImGui::Begin("Registers and Flags");
		{
			float h = ImGui::GetContentRegionAvail().y * 0.75f;
            ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
            ImGui::BeginChild("##rfL", ImVec2(ImGui::GetContentRegionAvail().x * 0.25f, h), true, window_flags);
			ImGui::Text("Flags");
			if (ImGui::BeginTable("Flags", 2)) {
				ImGui::TableSetupColumn("Name");
				ImGui::TableSetupColumn("Value");
				ImGui::TableHeadersRow();
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Text("S");
				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%d", (REGISTER(F) & FLAG_S) != 0);
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Text("Z");
				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%d", (REGISTER(F) & FLAG_Z) != 0);
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Text("AC");
				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%d", (REGISTER(F) & FLAG_AC) != 0);
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Text("P");
				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%d", (REGISTER(F) & FLAG_P) != 0);
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Text("CY");
				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%d", (REGISTER(F) & FLAG_CY) != 0);
				ImGui::EndTable();
			}
            ImGui::EndChild();
			ImGui::SameLine();
            ImGui::BeginChild("##rfR", ImVec2(0, h), true, window_flags);
			ImGui::Text("Registers");
			if (ImGui::BeginTable("Registers", 2)) {
				ImGui::TableSetupColumn("Name");
				ImGui::TableSetupColumn("Value");
				ImGui::TableHeadersRow();
				ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("A");
					ImGui::TableSetColumnIndex(1);
					ImGui::Text("%.2X", REGISTER(A));
				ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("B");
					ImGui::TableSetColumnIndex(1);
					ImGui::Text("%.2X", REGISTER(B));
				ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("C");
					ImGui::TableSetColumnIndex(1);
					ImGui::Text("%.2X", REGISTER(C));
				ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("D");
					ImGui::TableSetColumnIndex(1);
					ImGui::Text("%.2X", REGISTER(D));
				ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("E");
					ImGui::TableSetColumnIndex(1);
					ImGui::Text("%.2X", REGISTER(E));
				ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("F");
					ImGui::TableSetColumnIndex(1);
					ImGui::Text("%.2X", REGISTER(F));
				ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("H");
					ImGui::TableSetColumnIndex(1);
					ImGui::Text("%.2X", REGISTER(H));
				ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("L");
					ImGui::TableSetColumnIndex(1);
					ImGui::Text("%.2X", REGISTER(L));
				ImGui::EndTable();
			}
            ImGui::EndChild();
            ImGui::BeginChild("##rfB", ImVec2(0, 0), true, window_flags);
			if (ImGui::BeginTable("Registers", 2)) {
				ImGui::TableSetupColumn("Name");
				ImGui::TableSetupColumn("Value");
				ImGui::TableHeadersRow();
				ImGui::TableNextRow();
				ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("SP");
					ImGui::TableSetColumnIndex(1);
					ImGui::Text("%.4X", SP);
				ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("PC");
					ImGui::TableSetColumnIndex(1);
					ImGui::Text("%.4X", PC);
				ImGui::EndTable();
			}
			ImGui::EndChild();
        }
		ImGui::End();

		ImGui::Begin("Stack");
		{
            ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
            ImGui::BeginChild("##stackT", ImVec2(0, 58), true, window_flags);

			ImGui::Text("Start (Hex)");
			ImGui::SameLine();
			ImGui::InputScalar("##stkhex", ImGuiDataType_U16, &stkstart, NULL, NULL, "%x");
			ImGui::Text("Start (Dec)");
			ImGui::SameLine();
			ImGui::InputScalar("##stkdec", ImGuiDataType_U16, &stkstart, NULL, NULL, "%u");

			stkstart = (stkstart > SP) ? SP : stkstart;

            ImGui::EndChild();
            ImGui::BeginChild("##stackB", ImVec2(0, 0), true, window_flags);

			if (ImGui::BeginTable("##stack", 4, flags)) {
				ImGui::TableSetupColumn("Address (Hex)");
				ImGui::TableSetupColumn("Address (Dec)");
				ImGui::TableSetupColumn("Data (Hex)");
				ImGui::TableSetupColumn("Data (Dec)");
				ImGui::TableHeadersRow();
				int stkend = stkstart - 0x100 > 0 ? stkstart - 0x100 : 0;
				for (int row = stkstart; row >= stkend; row--) {
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("%.4XH", row);
					ImGui::TableSetColumnIndex(1);
					ImGui::Text("%d",  row);
					ImGui::TableSetColumnIndex(2);
					ImGui::Text("%.2XH", memory[row]);
					ImGui::TableSetColumnIndex(3);
					ImGui::Text("%d", memory[row]);
				}
				ImGui::EndTable();
			}
            ImGui::EndChild();
		}
		ImGui::End();

		ImGui::Begin("Memory");
		{
            ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
            ImGui::BeginChild("##memoryT", ImVec2(0, 58), true, window_flags);

			ImGui::Text("Start (Hex)");
			ImGui::SameLine();
			ImGui::InputScalar("##memhex", ImGuiDataType_U16, &memstart, NULL, NULL, "%x");
			ImGui::Text("Start (Dec)");
			ImGui::SameLine();
			ImGui::InputScalar("##memdec", ImGuiDataType_U16, &memstart, NULL, NULL, "%u");

            ImGui::EndChild();
            ImGui::BeginChild("##memoryB", ImVec2(0, 0), true, window_flags);

			int memend = memstart + 0x100 < 0x10000 ? memstart + 0x100 : 0x10000;
			if (ImGui::BeginTable("##memory", 4, flags)) {
				ImGui::TableSetupColumn("Address (Hex)");
				ImGui::TableSetupColumn("Address (Dec)");
				ImGui::TableSetupColumn("Data (Hex)");
				ImGui::TableSetupColumn("Data (Dec)");
				ImGui::TableHeadersRow();
				for (int row = memstart; row < memend; row++) {
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("%.4XH", row);
					ImGui::TableSetColumnIndex(1);
					ImGui::Text("%d", row);
					ImGui::TableSetColumnIndex(2);
					ImGui::Text("%.2XH", memory[row]);
					ImGui::TableSetColumnIndex(3);
					ImGui::Text("%d", memory[row]);
				}
				ImGui::EndTable();
			}
            ImGui::EndChild();
		}
		ImGui::End();

		ImGui::Begin("IO Ports");
		{
            ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
            ImGui::BeginChild("##ioT", ImVec2(0, 58), true, window_flags);

			ImGui::Text("Start (Hex)");
			ImGui::SameLine();
			ImGui::InputScalar("##iohex", ImGuiDataType_U8, &iostart, NULL, NULL, "%x");
			ImGui::Text("Start (Dec)");
			ImGui::SameLine();
			ImGui::InputScalar("##iodec", ImGuiDataType_U8, &iostart, NULL, NULL, "%u");

            ImGui::EndChild();
            ImGui::BeginChild("##ioB", ImVec2(0, 0), true, window_flags);

			if (ImGui::BeginTable("##io", 4, flags)) {
				ImGui::TableSetupColumn("Address (Hex)");
				ImGui::TableSetupColumn("Address (Dec)");
				ImGui::TableSetupColumn("Data (Hex)");
				ImGui::TableSetupColumn("Data (Dec)");
				ImGui::TableHeadersRow();

				for (uint16_t row = iostart; row < 0x100; row++) {
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("%.2XH", row);
					ImGui::TableSetColumnIndex(1);
					ImGui::Text("%d"  , row);
					ImGui::TableSetColumnIndex(2);
					ImGui::Text("%.2XH", IO[row]);
					ImGui::TableSetColumnIndex(3);
					ImGui::Text("%d", IO[row]);
				}
				ImGui::EndTable();
			}
            ImGui::EndChild();
		}
		ImGui::End();

		ImGui::Begin("Assembler");
		ImGui::End();
		ImGui::PopStyleVar();

        // Rendering
        ImGui::Render();
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);

		if (!program_should_run || !no_errors) continue;

		switch (memory[PC]) {

#define PUSH(rp) \
		case PUSH_ ##rp: \
			memory[--SP] = UPPER_BYTE_ ##rp; \
			memory[--SP] = LOWER_BYTE_ ##rp; \
			PC++; \
			break

			PUSH(B);
			PUSH(D);
			PUSH(H);
			PUSH(PSW);
#undef PUSH

#define POP(rp) \
		case POP_ ##rp: \
			LOWER_BYTE_ ##rp = memory[SP++]; \
			UPPER_BYTE_ ##rp = memory[SP++]; \
			PC++; \
			break

			POP(B);
			POP(D);
			POP(H);
			POP(PSW);
#undef POP

		case XTHL:
			WZ = REGISTER(L);
			REGISTER(L) = memory[SP];
			memory[SP] = WZ;
			WZ = REGISTER(H);
			REGISTER(H) = memory[SP + 1];
			memory[SP + 1] = WZ;
			++PC; break;

		case SPHL:
			SP = REG_PAIR(H);
			++PC; break;

#define MVI(x) \
	case MVI_ ##x : \
		REGISTER(x) = memory[++PC]; \
		++PC; \
		break

			MVI(A);
			MVI(B);
			MVI(C);
			MVI(D);
			MVI(E);
			MVI(H);
			MVI(L);
			MVI(M);
#undef MVI

#define MOV(x, y) \
	case MOV_ ##x## _ ##y : \
		REGISTER(x) = REGISTER(y); \
		++PC; \
		break

			MOV(A, A); MOV(B, A); MOV(C, A); MOV(D, A);
			MOV(A, B); MOV(B, B); MOV(C, B); MOV(D, B);
			MOV(A, C); MOV(B, C); MOV(C, C); MOV(D, C);
			MOV(A, D); MOV(B, D); MOV(C, D); MOV(D, D);
			MOV(A, E); MOV(B, E); MOV(C, E); MOV(D, E);
			MOV(A, H); MOV(B, H); MOV(C, H); MOV(D, H);
			MOV(A, L); MOV(B, L); MOV(C, L); MOV(D, L);
			MOV(A, M); MOV(B, M); MOV(C, M); MOV(D, M);

			MOV(E, A); MOV(H, A); MOV(L, A); MOV(M, A);
			MOV(E, B); MOV(H, B); MOV(L, B); MOV(M, B);
			MOV(E, C); MOV(H, C); MOV(L, C); MOV(M, C);
			MOV(E, D); MOV(H, D); MOV(L, D); MOV(M, D);
			MOV(E, E); MOV(H, E); MOV(L, E); MOV(M, E);
			MOV(E, H); MOV(H, H); MOV(L, H); MOV(M, H);
			MOV(E, L); MOV(H, L); MOV(L, L); MOV(M, L);
			MOV(E, M); MOV(H, M); MOV(L, M);
#undef MOV

		case LDAX_B:
			REGISTER(A) = memory[REG_PAIR(B)];
			++PC; break;

		case LDAX_D:
			REGISTER(A) = memory[REG_PAIR(D)];
			++PC; break;

		case STAX_B:
			memory[REG_PAIR(B)] = REGISTER(A);
			++PC; break;

		case STAX_D:
			memory[REG_PAIR(D)] = REGISTER(A);
			++PC; break;

		case LHLD:
			WZ = (memory[PC + 1] & 0xFF) | ((memory[PC + 2] & 0xFF) << 8);
			REGISTER(L) = memory[WZ];
			REGISTER(H) = memory[WZ + 1];
			PC += 3; break;

		case SHLD:
			WZ = (memory[PC + 1] & 0xFF) | ((memory[PC + 2] & 0xFF) << 8);
			memory[WZ] = REGISTER(L);
			memory[WZ + 1] = REGISTER(H);
			PC += 3; break;

		case LDA:
			WZ = (memory[PC + 1] & 0xFF) | ((memory[PC + 2] & 0xFF) << 8);
			REGISTER(A) = memory[WZ];
			PC += 3; break;

		case STA:
			WZ = (memory[PC + 1] & 0xFF) | ((memory[PC + 2] & 0xFF) << 8);
			memory[WZ] = REGISTER(A);
			PC += 3; break;

#define LXI(rp) \
		case LXI_ ##rp: \
			LOWER_BYTE_ ##rp = memory[++PC]; \
			UPPER_BYTE_ ##rp = memory[++PC]; \
			++PC; \
			break

		LXI(B);
		LXI(D);
		LXI(H);
		LXI(SP);
#undef LXI

		case XCHG:
			WZ = REGISTER(L);
			REGISTER(L) = REGISTER(E);
			REGISTER(E) = WZ;
			WZ = REGISTER(H);
			REGISTER(H) = REGISTER(D);
			REGISTER(D) = WZ;
			++PC; break;

#define ADD(x) \
		case ADD_ ##x: { \
			REGISTER(A) = ALU(REGISTER(A), REGISTER(x), '+', 1); \
			PC ++; \
		} break

			ADD(A);
			ADD(B);
			ADD(C);
			ADD(D);
			ADD(E);
			ADD(H);
			ADD(L);
			ADD(M);
#undef ADD

		case ADI: {
			REGISTER(A) = ALU(REGISTER(A), memory[++PC], '+', 1);
			PC ++;
		} break;

#define ADC(x) \
		case ADC_ ##x: { \
			REGISTER(A) = ALU(REGISTER(A), REGISTER(x) + ((REGISTER(F) & FLAG_CY) != 0), '+', 1); \
			PC ++; \
		} break

			ADC(A);
			ADC(B);
			ADC(C);
			ADC(D);
			ADC(E);
			ADC(H);
			ADC(L);
			ADC(M);
#undef ADC

		case ACI: {
			REGISTER(A) = ALU(REGISTER(A), memory[++PC] + ((REGISTER(F) & FLAG_CY) != 0), '+', 1);
			PC ++;
		} break;

#define SUB(x) \
		case SUB_ ##x: { \
			REGISTER(A) = ALU(REGISTER(A), REGISTER(x), '-', 1); \
			PC ++; \
		} break

			SUB(A);
			SUB(B);
			SUB(C);
			SUB(D);
			SUB(E);
			SUB(H);
			SUB(L);
			SUB(M);
#undef SUB

		case SUI: {
			REGISTER(A) = ALU(REGISTER(A), memory[++PC], '-', 1);
			PC ++;
		} break;

#define SBB(x) \
		case SBB_ ##x: { \
			REGISTER(A) = ALU(REGISTER(A), REGISTER(x) + ((REGISTER(F) & FLAG_CY) != 0), '-', 1); \
			PC ++; \
		} break

			SBB(A);
			SBB(B);
			SBB(C);
			SBB(D);
			SBB(E);
			SBB(H);
			SBB(L);
			SBB(M);
#undef SBB

		case SBI: {
			REGISTER(A) = ALU(REGISTER(A), memory[++PC] + ((REGISTER(F) & FLAG_CY) != 0), '-', 1);
			PC ++;
		} break;

#define INR(x) \
		case INR_ ##x: { \
			REGISTER(x) = ALU(REGISTER(x), 1, '-', 0); \
			PC++; \
		} break

			INR(A);
			INR(B);
			INR(C);
			INR(D);
			INR(E);
			INR(H);
			INR(L);
			INR(M);
#undef INR

#define DCR(x) \
		case DCR_ ##x: { \
			REGISTER(x) = ALU(REGISTER(x), 1, '-', 0); \
			PC++; \
		} break

			DCR(A);
			DCR(B);
			DCR(C);
			DCR(D);
			DCR(E);
			DCR(H);
			DCR(L);
			DCR(M);
#undef DCR

#define INX(rp) \
		case INX_ ##rp: \
			UPPER_BYTE_ ##rp += (++LOWER_BYTE_ ##rp == 0x00); \
			PC++; \
			break

			INX(B);
			INX(D);
			INX(H);
			INX(SP);
#undef INX

#define DCX(rp) \
		case DCX_ ##rp: \
			UPPER_BYTE_ ##rp -= (--LOWER_BYTE_ ##rp == 0xff); \
			PC++; \
			break

			DCX(B);
			DCX(D);
			DCX(H);
			DCX(SP);
#undef DCX

#define DAD(rp) \
		case DAD_ ##rp: { \
			LOWER_BYTE_H += LOWER_BYTE_ ##rp; \
			char ic = (LOWER_BYTE_H + LOWER_BYTE_ ##rp) > 0xFF;\
			UPPER_BYTE_H += UPPER_BYTE_ ##rp; \
			UPPER_BYTE_H += ic; \
			(REG_PAIR(rp) + REG_PAIR(H) > 0xFFFF) ? \
				SET(REGISTER(F), FLAG_CY): \
				RESET(REGISTER(F), FLAG_CY); \
		} break

			DAD(B);
			DAD(D);
			DAD(H);
			DAD(SP);
#undef DAD

#define CMP(x) \
		case CMP_ ##x: { \
			ALU(REGISTER(A), REGISTER(x), '-', 1); \
			PC++; \
		} break

			CMP(A);
			CMP(B);
			CMP(C);
			CMP(D);
			CMP(E);
			CMP(H);
			CMP(L);
			CMP(M);
#undef CMP

		case DAA: {
			uint8_t un = (REGISTER(A) >> 4) & 0xF;
			uint8_t ln = (REGISTER(A) >> 0) & 0xF;
			if (ln > 0x9 || REGISTER(F) & FLAG_AC) {
				ln += 0x06;
				un += (ln > 0x0f);
				un %= 0x10;
				if (un == 0) SET(REGISTER(F), FLAG_CY);
				ln %= 0x10;
			}
			if (un > 0x9 || REGISTER(F) & FLAG_CY) {
				un += 0x06;
				if (un > 0x0f)
					SET(REGISTER(F), FLAG_CY);
				un %= 0x10;
			}
			REGISTER(A) = ln | un << 4;
		} ++PC; break;

		case RAL: {
			uint8_t c = (REGISTER(F) & FLAG_CY) != 0;
			(REGISTER(A) & 0x80) ?
				SET(REGISTER(F), FLAG_CY):
				RESET(REGISTER(F), FLAG_CY);
			REGISTER(A) <<= 1;
			REGISTER(A) |= c;
		} PC++; break;

		case RAR: {
			uint8_t c = (REGISTER(F) & FLAG_CY) != 0;
			(REGISTER(A) & 0x01) ?
				SET(REGISTER(F), FLAG_CY):
				RESET(REGISTER(F), FLAG_CY);
			REGISTER(A) >>= 1;
			REGISTER(A) |= (c << 7);
		} PC++; break;

		case RLC: {
			uint8_t c = (REGISTER(A) & 0x80) != 0;
			(c) ?
				SET(REGISTER(F), FLAG_CY):
				RESET(REGISTER(F), FLAG_CY);
			REGISTER(A) <<= 1;
			REGISTER(A) |= c;
		} PC++; break;

		case RRC: {
			uint8_t c = (REGISTER(A) & 0x01) != 0;
			(c) ?
				SET(REGISTER(F), FLAG_CY):
				RESET(REGISTER(F), FLAG_CY);
			REGISTER(A) >>= 1;
			REGISTER(A) |= (c << 7);
		} PC++; break;

#define ANA(x) \
		case ANA_ ##x: \
			REGISTER(A) = ALU(REGISTER(A), REGISTER(x), '&', 0); \
			SET(REGISTER(F), FLAG_AC); \
			RESET(REGISTER(F), FLAG_CY); \
			PC ++; break

			ANA(A);
			ANA(B);
			ANA(C);
			ANA(D);
			ANA(E);
			ANA(H);
			ANA(L);
			ANA(M);
#undef ANA

		case ANI:
			REGISTER(A) = ALU(REGISTER(A), memory[++PC], '&', 0);
			SET(REGISTER(F), FLAG_AC);
			RESET(REGISTER(F), FLAG_CY);
			PC ++; break;

#define XRA(x) \
		case XRA_ ##x: \
			REGISTER(A) = ALU(REGISTER(A), REGISTER(x), '^', 0); \
			RESET(REGISTER(F), FLAG_AC); \
			RESET(REGISTER(F), FLAG_CY); \
			PC ++; break

			XRA(A);
			XRA(B);
			XRA(C);
			XRA(D);
			XRA(E);
			XRA(H);
			XRA(L);
			XRA(M);
#undef XRA

		case XRI:
			REGISTER(A) = ALU(REGISTER(A), memory[++PC], '^', 0);
			RESET(REGISTER(F), FLAG_AC);
			RESET(REGISTER(F), FLAG_CY);
			PC ++; break;

#define ORA(x) \
		case ORA_ ##x: \
			REGISTER(A) = ALU(REGISTER(A), REGISTER(x), '|', 0); \
			RESET(REGISTER(F), FLAG_AC); \
			RESET(REGISTER(F), FLAG_CY); \
			PC ++; break

			ORA(A);
			ORA(B);
			ORA(C);
			ORA(D);
			ORA(E);
			ORA(H);
			ORA(L);
			ORA(M);
#undef ORA

		case ORI:
			REGISTER(A) = ALU(REGISTER(A), memory[++PC], '|', 0);
			RESET(REGISTER(F), FLAG_AC);
			RESET(REGISTER(F), FLAG_CY);
			PC ++; break;

		case CMA:
			REGISTER(A) = ~REGISTER(A);
			++PC; break;

		case STC:
			SET(REGISTER(F), FLAG_CY);
			++PC; break;

		case CMC:
			TOGGLE(REGISTER(F), FLAG_CY);
			++PC; break;

		case CPI: {
			ALU(REGISTER(A), memory[++PC], '-', 1);
			PC++;
		} break;

		case JMP:
			PC = (uint16_t)memory[PC + 2] << 8 | memory[PC + 1];
			break;

#define JMP_ON_TRUE(flag) \
		PC = (REGISTER(F) & FLAG_ ##flag) ? \
			(uint16_t)memory[PC + 2] << 8 | memory[PC + 1] : PC + 3; \
		break

		case JZ : JMP_ON_TRUE(Z);
		case JPE: JMP_ON_TRUE(P);
		case JC : JMP_ON_TRUE(CY);
		case JM : JMP_ON_TRUE(S);
#undef JMP_ON_TRUE

#define JMP_ON_FALSE(flag) \
		PC = (REGISTER(F) & FLAG_ ##flag) ? \
			PC + 3 : (uint16_t)memory[PC + 2] << 8 | memory[PC + 1]; \
		break

		case JNZ: JMP_ON_FALSE(Z);
		case JPO: JMP_ON_FALSE(P);
		case JNC: JMP_ON_FALSE(CY);
		case JP : JMP_ON_FALSE(S);
#undef JMP_ON_FALSE

		case CALL:
			memory[--SP] = PC >> 8;
			memory[--SP] = PC & 0xff;
			PC = (uint16_t)memory[PC + 2] << 8 | memory[PC + 1];
			break;

#define CALL_ON_TRUE(flag) \
		if (REGISTER(F) & FLAG_ ##flag) { \
			memory[--SP] = PC >> 8; \
			memory[--SP] = PC & 0xff; \
			PC = (uint16_t)memory[PC + 2] << 8 | memory[PC + 1]; \
		} else { \
			PC += 3;\
		} break

		case CZ : CALL_ON_TRUE(Z);
		case CPE: CALL_ON_TRUE(P);
		case CC : CALL_ON_TRUE(CY);
		case CM : CALL_ON_TRUE(S);
#undef CALL_ON_TRUE

#define CALL_ON_FALSE(flag) \
		if (REGISTER(F) & FLAG_ ##flag) { \
			PC += 3;\
		} else { \
			memory[--SP] = PC >> 8; \
			memory[--SP] = PC & 0xff; \
			PC = (uint16_t)memory[PC + 2] << 8 | memory[PC + 1]; \
		} break

		case CNZ: CALL_ON_FALSE(Z);
		case CPO: CALL_ON_FALSE(P);
		case CNC: CALL_ON_FALSE(CY);
		case CP : CALL_ON_FALSE(S);
#undef CALL_ON_FALSE

		case RET:
			PC = (uint16_t)memory[SP + 1] << 8 | memory[SP];
			SP += 2;
			break;

#define RET_ON_TRUE(flag) \
		if (REGISTER(F) & FLAG_ ##flag) { \
			PC = (uint16_t)memory[SP + 1] << 8 | memory[SP]; \
			SP += 2; \
		} else { \
			PC += 3; \
		} break

		case RZ : RET_ON_TRUE(Z);
		case RPE: RET_ON_TRUE(P);
		case RC : RET_ON_TRUE(CY);
		case RM : RET_ON_TRUE(S);
#undef RET_ON_TRUE

#define RET_ON_FALSE(flag) \
		if (REGISTER(F) & FLAG_ ##flag) { \
			PC += 3; \
		} else { \
			PC = (uint16_t)memory[SP + 1] << 8 | memory[SP]; \
			SP += 2; \
		} break

		case RNZ: RET_ON_FALSE(Z);
		case RPO: RET_ON_FALSE(P);
		case RNC: RET_ON_FALSE(CY);
		case RP : RET_ON_FALSE(S);
#undef RET_ON_FALSE

		case PCHL:
			PC = REG_PAIR(H);
			break;

		case RST_0:
		case RST_1:
		case RST_2:
		case RST_3:
		case RST_4:
		case RST_5:
		case RST_6:
		case RST_7:
		case HLT:
			program_should_run = 0;
			++PC;
			break;

		case EI:
		case DI:
		case NOP:
			++PC; break;

		case IN:
			REGISTER(A) = IO[++PC];
			++PC; break;

		case OUT:
			IO[++PC] = REGISTER(A);
			++PC; break;

		default:
			fprintf(stderr, "[FATAL] unexpected opcode %0x\n", memory[PC]);
			program_should_run = 0;
			break;
		}
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

	return 0;
}
