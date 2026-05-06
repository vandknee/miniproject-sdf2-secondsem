#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>
#include <ctime>
#include <cstdlib>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;

// ================= DIFFICULTY =================
enum Difficulty {
    EASY = 1,
    MEDIUM,
    HARD
};

// ================= PLAYER =================
class Player {
    string name;
    int score;

public:
    Player(string n) : name(n), score(0) {}

    void addScore(int s) { score += s; }
    int getScore() { return score; }
    string getName() { return name; }
};

// ================= IMAGE BASE =================
class PixelImage {
protected:
    int width, height, channels;
    unsigned char* img;

public:
    PixelImage(string path) {
        img = stbi_load(path.c_str(), &width, &height, &channels, 0);
        if (!img) cout << "Image not found\n";
    }

    virtual ~PixelImage() {
        if (img) stbi_image_free(img);
    }
};

// ================= ASCII =================
class ASCIIImage : public PixelImage {

public:
    ASCIIImage(string path) : PixelImage(path) {}

    void renderHalf() {
        int scale = max(1, height / 40);

        for (int y = 0; y < height; y += scale) {
            for (int x = 0; x < width / 2; x += scale) {

                int i = (y * width + x) * channels;
                int gray = (img[i] + img[i+1] + img[i+2]) / 3;

                cout << (gray > 200 ? ' ' :
                         gray > 150 ? '.' :
                         gray > 100 ? '*' : '#');
            }
            cout << "\n";
        }
    }

    void renderFull() {
        int scale = max(1, height / 40);

        for (int y = 0; y < height; y += scale) {
            for (int x = 0; x < width; x += scale) {

                int i = (y * width + x) * channels;
                int gray = (img[i] + img[i+1] + img[i+2]) / 3;

                cout << (gray > 200 ? ' ' :
                         gray > 150 ? '.' :
                         gray > 100 ? '*' : '#');
            }
            cout << "\n";
        }
    }
};

// ================= COLOR =================
class ColorImage : public PixelImage {

public:
    ColorImage(string path) : PixelImage(path) {}

    void render() {
        int scale = max(1, height / 40);

        for (int y = 0; y < height; y += scale) {
            for (int x = 0; x < width; x += scale) {

                int i = (y * width + x) * channels;

                cout << "\033[48;2;"
                     << (int)img[i] << ";"
                     << (int)img[i+1] << ";"
                     << (int)img[i+2] << "m  ";
            }
            cout << "\033[0m\n";
        }
    }
};

// ================= HINT =================
class Hint {
public:
    virtual void show(string answer) = 0;
};

class FirstLetterHint : public Hint {
public:
    void show(string answer) override {
        cout << "Hint: Starts with " << answer[0] << "\n";
    }
};

// ================= QUESTION =================
class Question {
public:
    string clue;
    vector<string> options;
    int answer;
    string logo;
    vector<Hint*> hints;
};

// ================= SCORE =================
class ScoreManager {
public:
    void save(Player& p) {
        ofstream f("scores.txt", ios::app);
        f << p.getName() << " " << p.getScore() << "\n";
    }

    void show() {
        ifstream f("scores.txt");

        vector<pair<string,int>> data;
        string name;
        int score;

        while (f >> name >> score)
            data.push_back({name, score});

        sort(data.begin(), data.end(), [](auto &a, auto &b){
            return a.second > b.second;
        });

        cout << "\nLeaderboard\n----------\n";
        for (int i = 0; i < min(5,(int)data.size()); i++)
            cout << data[i].first << " " << data[i].second << "\n";
    }
};

// ================= GAME =================
class Game {

    vector<Question> questions;
    Difficulty diff;
    string gameMode;

public:
    Game() {}

    void selectGameMode() {
        int mode;
        cout << "\n========== GAME MODE SELECTION ==========\n";
        cout << "1. Logo Guessing\n";
        cout << "2. SDG (Sustainable Development Goals) Guessing\n";
        cout << "Enter your choice (1-2): ";
        
        while (true) {
            if (!(cin >> mode)) {
                cout << "Invalid input! Enter 1 or 2: ";
                cin.clear();
                cin.ignore(10000, '\n');
                continue;
            }
            
            if (mode == 1) {
                gameMode = "logo";
                break;
            } else if (mode == 2) {
                gameMode = "sdg";
                break;
            } else {
                cout << "Choose 1 or 2: ";
            }
        }
        
        load();
    }

    void load() {
        ifstream file("questions.txt");
        string line;
        vector<Question> allQuestions;

        while (getline(file, line)) {
            if (line.empty()) continue;

            // Parse the line: mode|clue|option1,option2,option3,option4|answer|logo
            Question q;
            
            size_t pos0 = line.find('|');
            size_t pos1 = line.find('|', pos0 + 1);
            size_t pos2 = line.find('|', pos1 + 1);
            size_t pos3 = line.find('|', pos2 + 1);

            string mode = line.substr(0, pos0);
            
            // Skip if this question is not for the selected mode
            if (mode != gameMode) continue;
            
            q.clue = line.substr(pos0 + 1, pos1 - pos0 - 1);
            
            string optionsStr = line.substr(pos1 + 1, pos2 - pos1 - 1);
            q.answer = stoi(line.substr(pos2 + 1, pos3 - pos2 - 1));
            q.logo = line.substr(pos3 + 1);

            // Parse options
            size_t optPos = 0;
            while (optPos < optionsStr.length()) {
                size_t commaPos = optionsStr.find(',', optPos);
                if (commaPos == string::npos) {
                    q.options.push_back(optionsStr.substr(optPos));
                    break;
                }
                q.options.push_back(optionsStr.substr(optPos, commaPos - optPos));
                optPos = commaPos + 1;
            }

            q.hints.push_back(new FirstLetterHint());
            allQuestions.push_back(q);
        }

        file.close();

        // Select 5 random questions
        if (allQuestions.size() > 5) {
            random_shuffle(allQuestions.begin(), allQuestions.end());
            questions.assign(allQuestions.begin(), allQuestions.begin() + 5);
        } else {
            questions = allQuestions;
        }
    }

    void selectDifficulty() {
        int d;
        cout << "\nSelect Difficulty:\n1 Easy\n2 Medium\n3 Hard\n";
        cin >> d;

        if (d < 1 || d > 3) d = 2;
        diff = (Difficulty)d;
    }

    int getValidInput(int maxOption) {
        int ans;

        while (true) {
            cout << "\nYour answer: ";

            if (!(cin >> ans)) {
                cout << "Invalid input! Enter a number.\n";
                cin.clear();
                cin.ignore(10000, '\n');
                continue;
            }

            if (ans < 1 || ans > maxOption) {
                cout << "Choose a valid option (1-" << maxOption << ")\n";
                continue;
            }

            return ans;
        }
    }

    void playRound(Question &q, Player &p) {

        ASCIIImage ascii(q.logo);
        ColorImage color(q.logo);

        int attempts = 0;
        int maxAttempts = (diff == HARD) ? 2 : 3;

        while (attempts < maxAttempts) {

            int ans = getValidInput(q.options.size());

            if (ans == q.answer) {
                cout << "Correct!\n";
                color.render();

                if (diff == EASY) p.addScore(5);
                else if (diff == MEDIUM) p.addScore(10);
                else p.addScore(15);

                return;
            }

            attempts++;

            if (attempts == 1 && diff != HARD) {
                cout << "Wrong! Half ASCII:\n";
                ascii.renderHalf();
            }
            else if (attempts == 2 && maxAttempts > 2) {
                cout << "Wrong! Full ASCII:\n";
                ascii.renderFull();

                if (diff != HARD)
                    q.hints[0]->show(q.options[q.answer-1]);
            }
            else {
                cout << "Final reveal:\n";
                color.render();
                cout << "Answer: " << q.options[q.answer-1] << "\n";

                if (diff == EASY) p.addScore(-2);
                else if (diff == MEDIUM) p.addScore(-5);
                else p.addScore(-10);
            }
        }
    }

    void start() {

        string name;
        cout << "Enter name: ";
        cin >> name;

        selectGameMode();
        selectDifficulty(); 

        Player p(name);
        
        string modeDisplay = (gameMode == "logo") ? "Logo Guessing" : "SDG (Sustainable Development Goals)";
        cout << "\n========== " << modeDisplay << " ==========\n";

        for (auto &q : questions) {

            cout << "\nClue: " << q.clue << "\n";
            for (int i = 0; i < q.options.size(); i++)
                cout << i+1 << ". " << q.options[i] << "\n";

            playRound(q, p);
        }

        cout << "\nFinal Score: " << p.getScore() << "\n";

        ScoreManager sm;
        sm.save(p);
        sm.show();
    }
};

// ================= MAIN =================
int main() {

    srand(time(0));

    Game g;
    g.start();

    return 0;
}