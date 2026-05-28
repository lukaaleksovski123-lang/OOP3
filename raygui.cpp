#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include <vector>
#include <string>
#include <fstream>
#include <sstream>

class TextBuffer {
private:
    std::vector<std::string> lines;
    int cursorRow;
    int cursorCol;

public:
    TextBuffer() {
        lines.push_back("");
        cursorRow = 0;
        cursorCol = 0;
    }

    void Clear() {
        lines.clear();
        lines.push_back("");
        cursorRow = 0;
        cursorCol = 0;
    }

    void WriteChar(char c) {
        lines[cursorRow].insert(cursorCol, 1, c);
        cursorCol++;
    }

    void InsertNewLine() {
        std::string rightPart = lines[cursorRow].substr(cursorCol);
        lines[cursorRow] = lines[cursorRow].substr(0, cursorCol);
        lines.insert(lines.begin() + cursorRow + 1, rightPart);
        cursorRow++;
        cursorCol = 0;
    }

    void EraseChar() {
        if (cursorCol > 0) {
            lines[cursorRow].erase(cursorCol - 1, 1);
            cursorCol--;
        } else if (cursorRow > 0) {
            int previousRowLength = lines[cursorRow - 1].length();
            lines[cursorRow - 1] += lines[cursorRow];
            lines.erase(lines.begin() + cursorRow);
            cursorRow--;
            cursorCol = previousRowLength;
        }
    }

    bool SaveToFile(const std::string& filename) {
        std::ofstream outFile(filename);
        if (!outFile.is_open()) return false;
        for (size_t i = 0; i < lines.size(); i++) {
            outFile << lines[i];
            if (i < lines.size() - 1) outFile << "\n";
        }
        outFile.close();
        return true;
    }

    bool LoadFromFile(const std::string& filename) {
        std::ifstream inFile(filename);
        if (!inFile.is_open()) {
            Clear();
            return false;
        }

        lines.clear();
        std::string line;
        while (std::getline(inFile, line)) {
            lines.push_back(line);
        }
        if (lines.empty()) lines.push_back("");
        cursorRow = 0;
        cursorCol = 0;
        inFile.close();
        return true;
    }

    void MoveUp() {
        if (cursorRow > 0) {
            cursorRow--;
            if (cursorCol > (int)lines[cursorRow].length()) cursorCol = lines[cursorRow].length();
        }
    }

    void MoveDown() {
        if (cursorRow < (int)lines.size() - 1) {
            cursorRow++;
            if (cursorCol > (int)lines[cursorRow].length()) cursorCol = lines[cursorRow].length();
        }
    }

    void MoveLeft() {
        if (cursorCol > 0) {
            cursorCol--;
        } else if (cursorRow > 0) {
            cursorRow--;
            cursorCol = lines[cursorRow].length();
        }
    }

    void MoveRight() {
        if (cursorCol < (int)lines[cursorRow].length()) {
            cursorCol++;
        } else if (cursorRow < (int)lines.size() - 1) {
            cursorRow++;
            cursorCol = 0;
        }
    }

    int GetWordCount() const {
        int count = 0;
        for (const auto& line : lines) {
            std::stringstream ss(line);
            std::string word;
            while (ss >> word) {
                count++;
            }
        }
        return count;
    }

    const std::vector<std::string>& GetLines() const { return lines; }
    int GetCursorRow() const { return cursorRow; }
    int GetCursorCol() const { return cursorCol; }
};

class TextEditor {
private:
    TextBuffer buffer;
    const int screenWidth = 800;
    const int screenHeight = 600;
    std::string notificationMessage;
    float notificationTimer;
    float scrollValue;

    char filenameTextBoxBuffer[128];
    bool textBoxEditMode;

    void HandleInput() {
        if (!textBoxEditMode) {
            if (IsKeyPressed(KEY_ENTER)) buffer.InsertNewLine();
            if (IsKeyPressed(KEY_BACKSPACE)) buffer.EraseChar();

            if (IsKeyPressed(KEY_UP)) buffer.MoveUp();
            if (IsKeyPressed(KEY_DOWN)) buffer.MoveDown();
            if (IsKeyPressed(KEY_LEFT)) buffer.MoveLeft();
            if (IsKeyPressed(KEY_RIGHT)) buffer.MoveRight();

            int key = GetCharPressed();
            while (key > 0) {
                if ((key >= 32) && (key <= 125)) {
                    buffer.WriteChar((char)key);
                }
                key = GetCharPressed();
            }
        }

        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            scrollValue -= wheel;
            if (scrollValue < 0) scrollValue = 0;
        }

        if (notificationTimer > 0) {
            notificationTimer -= GetFrameTime();
        }
    }

    void DrawEditor() {
        BeginDrawing();
        ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

        GuiStatusBar(Rectangle{ 0, 0, (float)screenWidth, 40 }, "OOP Notepad");

        if (GuiButton(Rectangle{ 10, 7, 80, 25 }, "#02# Save")) {
            if (buffer.SaveToFile(filenameTextBoxBuffer)) {
                notificationMessage = "Saved to " + std::string(filenameTextBoxBuffer);
            } else {
                notificationMessage = "Save Error!";
            }
            notificationTimer = 3.0f;
        }

        if (GuiButton(Rectangle{ 100, 7, 80, 25 }, "#04# Open")) {
            if (buffer.LoadFromFile(filenameTextBoxBuffer)) {
                notificationMessage = "Loaded " + std::string(filenameTextBoxBuffer);
            } else {
                notificationMessage = "File not found! Created new.";
            }
            notificationTimer = 3.0f;
        }

        if (GuiTextBox(Rectangle{ 190, 7, 180, 25 }, filenameTextBoxBuffer, 128, textBoxEditMode)) {
            textBoxEditMode = !textBoxEditMode;
        }

        int startY = 60;
        int lineHeight = 22;
        int visibleLinesCount = (screenHeight - 80) / lineHeight;
        int totalLines = buffer.GetLines().size();

        int maxScrollValue = totalLines - visibleLinesCount;
        if (maxScrollValue < 0) maxScrollValue = 0;

        scrollValue = GuiScrollBar(Rectangle{ (float)screenWidth - 25, 45, 20, (float)screenHeight - 80 }, scrollValue, 0, (float)maxScrollValue);

        if (scrollValue < 0) scrollValue = 0;
        if (scrollValue > maxScrollValue) scrollValue = (float)maxScrollValue;

        int startLineIdx = (int)scrollValue;
        const auto& lines = buffer.GetLines();

        for (int i = startLineIdx; i < totalLines && (i - startLineIdx) < visibleLinesCount; i++) {
            int currentY = startY + ((i - startLineIdx) * lineHeight);
            DrawText(lines[i].c_str(), 20, currentY, 18, BLACK);

            if (!textBoxEditMode && i == buffer.GetCursorRow()) {
                std::string textBeforeCursor = lines[i].substr(0, buffer.GetCursorCol());
                int cursorXOffset = MeasureText(textBeforeCursor.c_str(), 18);

                if (((int)(GetTime() * 2) % 2) == 0) {
                    DrawLine(20 + cursorXOffset, currentY,
                             20 + cursorXOffset, currentY + 18, RED);
                }
            }
        }

        std::string statusText = "Row: " + std::to_string(buffer.GetCursorRow() + 1) +
                                 "  |  Col: " + std::to_string(buffer.GetCursorCol() + 1) +
                                 "  |  Words: " + std::to_string(buffer.GetWordCount());

        if (notificationTimer > 0) {
            statusText += "  |  [ " + notificationMessage + " ]";
        }

        GuiStatusBar(Rectangle{ 0, (float)screenHeight - 30, (float)screenWidth, 30 }, statusText.c_str());

        EndDrawing();
    }

public:
    void Run() {
        InitWindow(screenWidth, screenHeight, "OOP Text Editor");
        SetTargetFPS(60);

        notificationMessage = "";
        notificationTimer = 0.0f;
        scrollValue = 0.0f;

        TextCopy(filenameTextBoxBuffer, "untitled.txt");
        textBoxEditMode = false;

        while (!WindowShouldClose()) {
            HandleInput();
            DrawEditor();
        }

        CloseWindow();
    }
};

int main() {
    TextEditor editor;
    editor.Run();
    return 0;
}
