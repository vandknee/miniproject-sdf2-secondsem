#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <ctime>
#include <cstdlib>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;

// ================= BASE CLASS =================
class PixelImage {

protected:
    int width, height, channels;
    unsigned char* img;

public:
    PixelImage(string path) {
        img = stbi_load(path.c_str(), &width, &height, &channels, 0);

        if (img == NULL) {
            cout << "Image not found\n";
            width = height = 0;
        }
    }

    virtual ~PixelImage() {
        if (img) stbi_image_free(img);
    }
};

// ================= ASCII CLASS =================
class ASCIIImage : public PixelImage {

public:
    ASCIIImage(string path) : PixelImage(path) {}

    void renderHalfASCII() {

    int scale = height / 40;
    if (scale < 1) scale = 1;

    for (int y = 0; y < height; y += scale) {

        for (int x = 0; x < width / 2; x += scale) {   // 🔥 change here

            int idx = (y * width + x) * channels;

            int gray = (img[idx] + img[idx + 1] + img[idx + 2]) / 3;

            if (gray > 200) cout << " ";
            else if (gray > 150) cout << ".";
            else if (gray > 100) cout << "*";
            else cout << "#";
        }

        cout << "\n";
    }
}

    void renderFullASCII() {
        int scale = height / 40;
        if (scale < 1) scale = 1;

        for (int y = 0; y < height; y += scale) {
            for (int x = 0; x < width; x += scale) {
                int idx = (y * width + x) * channels;

                int gray = (img[idx] + img[idx + 1] + img[idx + 2]) / 3;

                if (gray > 200) cout << " ";
                else if (gray > 150) cout << ".";
                else if (gray > 100) cout << "*";
                else cout << "#";
            }
            cout << "\n";
        }
    }
};

// ================= COLOR CLASS =================
class ColorImage : public PixelImage {

public:
    ColorImage(string path) : PixelImage(path) {}

    void renderColor() {
        int scale = height / 40;
        if (scale < 1) scale = 1;

        for (int y = 0; y < height; y += scale) {
            for (int x = 0; x < width; x += scale) {

                int idx = (y * width + x) * channels;

                int r = img[idx];
                int g = img[idx + 1];
                int b = img[idx + 2];

                cout << "\033[48;2;" << r << ";" << g << ";" << b << "m  ";
            }
            cout << "\033[0m\n";
        }
    }
};

// ================= QUESTION =================
class Question {
public:
    string clue;
    vector<string> options;
    int answer;
    string logoFile;
};

// ================= SCORE =================
class ScoreManager {
public:
    void saveScore(string name, int score) {
        ofstream file("scores.txt", ios::app);
        file << name << " " << score << "\n";
        file.close();
    }

    void showLeaderboard() {
        ifstream file("scores.txt");

        string name;
        int score;

        cout << "\nLeaderboard\n------------------\n";

        while (file >> name >> score) {
            cout << name << " " << score << "\n";
        }

        file.close();
    }
};

// ================= GAME =================
class Game {

    vector<Question> questions;
    int score;

public:
    Game() {
        score = 0;
        loadQuestions();
    }

    void loadQuestions() {

        Question q1;
        q1.clue = "Electric car company founded by Elon Musk";
        q1.options = {"Tesla","Apple","Google","Samsung"};
        q1.answer = 1;
        q1.logoFile = "logos/tesla.jpg";

        Question q2;
        q2.clue = "Company famous for iPhone and Mac computers";
        q2.options = {"Microsoft","Apple","Intel","Sony"};
        q2.answer = 2;
        q2.logoFile = "logos/apple.png";

        questions.push_back(q1);
        questions.push_back(q2);
    }

    void startGame() {

        string name;
        cout << "Enter player name: ";
        cin >> name;

        for (auto q : questions) {

            cout << "\nClue: " << q.clue << "\n";

            for (int i = 0; i < q.options.size(); i++) {
                cout << i + 1 << ". " << q.options[i] << "\n";
            }

            ASCIIImage ascii(q.logoFile);
            ColorImage color(q.logoFile);

            int attempts = 0;
            bool correct = false;

            while (attempts < 3) {

                int ans;
                cout << "\nYour answer: ";
                cin >> ans;

                if (ans == q.answer) {
                    cout << "\nCorrect!\nShowing logo...\n";
                    color.renderColor();
                    score += 10;
                    correct = true;
                    break;
                }

                attempts++;

                if (attempts == 1) {
                    cout << "\nWrong! Showing HALF ASCII hint:\n";
                    ascii.renderHalfASCII();
                }
                else if (attempts == 2) {
                    cout << "\nWrong again! Showing FULL ASCII:\n";
                    ascii.renderFullASCII();
                }
                else {
                    cout << "\nWrong again! Final reveal:\n";
                    color.renderColor();
                    cout << "Answer: " << q.options[q.answer - 1] << "\n";
                    score -= 5;
                }
            }
        }

        cout << "\nFinal Score: " << score << "\n";

        ScoreManager sm;
        sm.saveScore(name, score);
        sm.showLeaderboard();
    }
};

// ================= MAIN =================
int main() {

    srand(time(0));

    Game game;

    int choice;

    while (true) {
        cout << "\n===== LogoQuest =====\n";
        cout << "1 Start Game\n2 Exit\n";
        cin >> choice;

        if (choice == 1)
            game.startGame();
        else
            break;
    }

    return 0;
}