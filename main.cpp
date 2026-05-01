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

public:
    Game() {
        load();
    }

    void load() {

        Question q1;
        q1.clue = "Electric car company founded by Elon Musk";
        q1.options = {"Tesla","Apple","Google","Samsung"};
        q1.answer = 1;
        q1.logo = "logos/tesla.jpg";
        q1.hints.push_back(new FirstLetterHint());

        Question q2;
        q2.clue = "Company famous for iPhone and Mac computers";
        q2.options = {"Microsoft","Apple","Intel","Sony"};
        q2.answer = 2;
        q2.logo = "logos/apple.png";
        q2.hints.push_back(new FirstLetterHint());

        questions.push_back(q1);
        questions.push_back(q2);

        random_shuffle(questions.begin(), questions.end());
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

        selectDifficulty();

        Player p(name);

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