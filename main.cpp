#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>
#include <ctime>
#include <cstdlib>
#include <random>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;

// ================= DIFFICULTY =================
enum Difficulty
{
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

    void addScore(int s) {

        score += s;

        // Prevent negative score
        if (score < 0)
            score = 0;
    }

    int getScore() {
        return score;
    }

    string getName() {
        return name;
    }
};

// ================= IMAGE BASE =================
class PixelImage {

protected:

    int width, height, channels;
    unsigned char* img;

public:

    PixelImage(string path) {

        img = stbi_load(path.c_str(),
                        &width,
                        &height,
                        &channels,
                        0);

        if (!img) {

            cout << "Image not found: "
                 << path << endl;

            width = height = channels = 0;
        }
    }

    virtual ~PixelImage() {

        if (img)
            stbi_image_free(img);
    }
};

// ================= ASCII IMAGE =================
class ASCIIImage : public PixelImage {

public:

    ASCIIImage(string path)
        : PixelImage(path) {}

    void renderHalf() {

        if (!img)
            return;

        int scale = max(1, height / 40);

        for (int y = 0;
             y < height;
             y += scale)
        {
            for (int x = 0;
                 x < width / 2;
                 x += scale)
            {
                int i =
                    (y * width + x) * channels;

                int gray =
                    (img[i] +
                     img[i + 1] +
                     img[i + 2]) / 3;

                cout << (gray > 200 ? ' ' :
                         gray > 150 ? '.' :
                         gray > 100 ? '*' : '#');
            }

            cout << "\n";
        }
    }

    void renderFull() {

        if (!img)
            return;

        int scale = max(1, height / 40);

        for (int y = 0;
             y < height;
             y += scale)
        {
            for (int x = 0;
                 x < width;
                 x += scale)
            {
                int i =
                    (y * width + x) * channels;

                int gray =
                    (img[i] +
                     img[i + 1] +
                     img[i + 2]) / 3;

                cout << (gray > 200 ? ' ' :
                         gray > 150 ? '.' :
                         gray > 100 ? '*' : '#');
            }

            cout << "\n";
        }
    }
};

// ================= COLOR IMAGE =================
class ColorImage : public PixelImage {

public:

    ColorImage(string path)
        : PixelImage(path) {}

    void render() {

        if (!img)
            return;

        int scale = max(1, height / 40);

        for (int y = 0;
             y < height;
             y += scale)
        {
            for (int x = 0;
                 x < width;
                 x += scale)
            {
                int i =
                    (y * width + x) * channels;

                cout << "\033[48;2;"
                     << (int)img[i] << ";"
                     << (int)img[i + 1] << ";"
                     << (int)img[i + 2]
                     << "m  ";
            }

            cout << "\033[0m\n";
        }
    }
};

// ================= HINT =================
class Hint {

public:

    virtual void show(string answer) = 0;

    virtual ~Hint() {}
};

class FirstLetterHint : public Hint {

public:

    void show(string answer) override {

        if (!answer.empty()) {

            cout << "Hint: Starts with "
                 << answer[0]
                 << "\n";
        }
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

        if (!f) {

            cout << "Unable to open scores file\n";
            return;
        }

        f << p.getName()
          << " "
          << p.getScore()
          << "\n";
    }

    void show() {

        ifstream f("scores.txt");

        if (!f) {

            cout << "No leaderboard found\n";
            return;
        }

        vector<pair<string, int>> data;

        string name;
        int score;

        while (f >> name >> score) {

            data.push_back({name, score});
        }

        sort(data.begin(),
             data.end(),
             [](auto &a, auto &b)
             {
                 return a.second > b.second;
             });

        cout << "\n========== LEADERBOARD ==========\n";

        for (int i = 0;
             i < min(5, (int)data.size());
             i++)
        {
            cout << i + 1
                 << ". "
                 << data[i].first
                 << " : "
                 << data[i].second
                 << "\n";
        }
    }
};

// ================= GAME =================
class Game {

    vector<Question> questions;

    Difficulty diff;

    string gameMode;

public:

    void selectGameMode() {

        int mode;

        cout << "\n========== GAME MODE ==========\n";
        cout << "1. Logo Guessing\n";
        cout << "2. SDG Guessing\n";

        while (true) {

            cout << "Enter choice: ";

            if (!(cin >> mode)) {

                cout << "Invalid input\n";

                cin.clear();
                cin.ignore(10000, '\n');

                continue;
            }

            if (mode == 1) {

                gameMode = "logo";
                break;
            }

            else if (mode == 2) {

                gameMode = "sdg";
                break;
            }

            else {

                cout << "Choose 1 or 2\n";
            }
        }

        load();
    }

    void load() {

        ifstream file("questions.txt");

        if (!file) {

            cout << "questions.txt not found\n";
            return;
        }

        string line;

        vector<Question> allQuestions;

        while (getline(file, line)) {

            if (line.empty())
                continue;

            Question q;

            size_t pos0 = line.find('|');
            size_t pos1 = line.find('|', pos0 + 1);
            size_t pos2 = line.find('|', pos1 + 1);
            size_t pos3 = line.find('|', pos2 + 1);

            // Invalid line protection
            if (pos0 == string::npos ||
                pos1 == string::npos ||
                pos2 == string::npos ||
                pos3 == string::npos)
            {
                continue;
            }

            string mode =
                line.substr(0, pos0);

            // Skip different mode questions
            if (mode != gameMode)
                continue;

            q.clue =
                line.substr(pos0 + 1,
                            pos1 - pos0 - 1);

            string optionsStr =
                line.substr(pos1 + 1,
                            pos2 - pos1 - 1);

            q.answer =
                stoi(line.substr(pos2 + 1,
                                 pos3 - pos2 - 1));

            q.logo =
                line.substr(pos3 + 1);

            // Parse options
            size_t optPos = 0;

            while (optPos < optionsStr.length()) {

                size_t commaPos =
                    optionsStr.find(',',
                                    optPos);

                if (commaPos == string::npos) {

                    q.options.push_back(
                        optionsStr.substr(optPos));

                    break;
                }

                q.options.push_back(
                    optionsStr.substr(
                        optPos,
                        commaPos - optPos));

                optPos = commaPos + 1;
            }

            // Validate answer index
            if (q.answer < 1 ||
                q.answer > q.options.size())
            {
                continue;
            }

            q.hints.push_back(
                new FirstLetterHint());

            allQuestions.push_back(q);
        }

        // Randomize questions
        if (allQuestions.size() > 5) {

            shuffle(allQuestions.begin(),
                    allQuestions.end(),
                    default_random_engine(time(0)));

            questions.assign(
                allQuestions.begin(),
                allQuestions.begin() + 5);
        }

        else {

            questions = allQuestions;
        }
    }

    void selectDifficulty() {

        int d;

        cout << "\nSelect Difficulty\n";
        cout << "1. Easy\n";
        cout << "2. Medium\n";
        cout << "3. Hard\n";

        cin >> d;

        if (d < 1 || d > 3)
            d = 2;

        diff = (Difficulty)d;
    }

    int getValidInput(int maxOption) {

        int ans;

        while (true) {

            cout << "\nYour answer: ";

            if (!(cin >> ans)) {

                cout << "Enter a valid number\n";

                cin.clear();
                cin.ignore(10000, '\n');

                continue;
            }

            if (ans < 1 ||
                ans > maxOption)
            {
                cout << "Choose between 1 and "
                     << maxOption
                     << "\n";

                continue;
            }

            return ans;
        }
    }

    void playRound(Question &q,
                   Player &p)
    {
        ASCIIImage ascii(q.logo);

        ColorImage color(q.logo);

        int attempts = 0;

        int maxAttempts =
            (diff == HARD) ? 1 : 3;

        while (attempts < maxAttempts) {

            int ans =
                getValidInput(
                    q.options.size());

            // Correct answer
            if (ans == q.answer) {

                cout << "Correct!\n";

                color.render();

                if (diff == EASY)
                    p.addScore(5);

                else if (diff == MEDIUM)
                    p.addScore(10);

                else
                    p.addScore(15);

                return;
            }

            attempts++;

            // HARD MODE
            if (diff == HARD) {

                cout << "Wrong!\n";

                cout << "Final reveal:\n";

                color.render();

                cout << "Answer: "
                     << q.options[q.answer - 1]
                     << "\n";

                p.addScore(-10);

                return;
            }

            // EASY + MEDIUM
            if (attempts == 1) {

                cout << "Wrong! Half ASCII:\n";

                ascii.renderHalf();
            }

            else if (attempts == 2) {

                cout << "Wrong! Full ASCII:\n";

                ascii.renderFull();

                q.hints[0]->show(
                    q.options[q.answer - 1]);
            }

            else {

                cout << "Final reveal:\n";

                color.render();

                cout << "Answer: "
                     << q.options[q.answer - 1]
                     << "\n";

                if (diff == EASY)
                    p.addScore(-2);

                else
                    p.addScore(-5);
            }
        }
    }

    void start() {

        string name;

        cout << "Enter name: ";

        cin.ignore();

        getline(cin, name);

        selectGameMode();

        selectDifficulty();

        Player p(name);

        string modeDisplay =
            (gameMode == "logo")
            ? "Logo Guessing"
            : "SDG Guessing";

        cout << "\n========== "
             << modeDisplay
             << " ==========\n";

        for (auto &q : questions) {

            cout << "\nClue: "
                 << q.clue
                 << "\n";

            for (int i = 0;
                 i < q.options.size();
                 i++)
            {
                cout << i + 1
                     << ". "
                     << q.options[i]
                     << "\n";
            }

            playRound(q, p);
        }

        cout << "\nFinal Score: "
             << p.getScore()
             << "\n";

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
