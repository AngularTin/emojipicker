// Compilation with linkages and flags 
// g++ -std=c++17 emojipicker.cpp -o emojipicker $(fltk-config --cflags --ldflags)

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Box.H>
#include <FL/fl_ask.H>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <string>
#include <cstdlib>
#include <unistd.h>
#include <linux/limits.h>

// stupid ass workaround
std::string getExecutableDir() {
    char buf[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf)-1);
    if (len != -1) {
        buf[len] = '\0';
        std::string path(buf);
        return path.substr(0, path.find_last_of('/'));
    }
    return ".";
}

// Emojies and their weighted searchterms
struct EmojiEntry {
    std::string emoji;
    std::unordered_map<std::string,int> weights;
};

std::vector<EmojiEntry> database;

// Simple parser to load the text emoji database
void load_db() {
    std::string path = getExecutableDir() + "/emojis.txt";
    std::ifstream f(path);
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string segment;
        EmojiEntry e;
        std::getline(ss, segment, ';');
        e.emoji = segment;
        while (std::getline(ss, segment, ';')) {
            auto pos = segment.find(':');
            if (pos != std::string::npos) {
                std::string key = segment.substr(0,pos);
                int w = std::stoi(segment.substr(pos+1));
                e.weights[key] = w;
            }
        }
        database.push_back(e);
    }
}

// Search for emojis given a query, accumulating scores for multiple terms
std::vector<std::string> search(const std::string &q_raw) {

    if (q_raw.empty()) {
        return std::vector<std::string>(10, ""); // 10 empty
    }
    
    // Sanetize the input and set letters to lowercase
    std::string q;
    q.reserve(q_raw.size());

    for (char c: q_raw) {
        int n = int(c);
        if (97 <= n && n <=122) {// lowercase
            q.push_back(c);
        } else if ( 65 <= n && n <= 90) {// Uppercase
            char lowercase = char(n+32);
            q.push_back(lowercase); 
        } else if (32 == n) {// Space
            q.push_back(c);
        }
    } 

    // Scores the emojies wrt. the search terms
    std::stringstream ss(q); // splits at space
    std::string term;
    std::vector<std::pair<int,std::string>> scored;

    for (auto &e : database) {
        int score = 0;
        std::stringstream ss2(q);
        while (ss2 >> term) {
            auto it = e.weights.find(term);
            if (it != e.weights.end()) {
                score += it->second;
            }
        }

        // Only save emojies with score >0
        if (score > 0) {
            scored.push_back({score, e.emoji});
        }
    }

    if (scored.empty())
        return std::vector<std::string>(10, "");
    
    // Sort wrt. score when there ar eto many element
    if (scored.size() > 10) {
        std::partial_sort(scored.begin(), scored.begin()+10, scored.end(),
            [](auto &a, auto &b){ return a.first > b.first; });
    }

    std::vector<std::string> res;
    for (int i = 0; i < 10 && i < (int)scored.size(); i++)
        res.push_back(scored[i].second);

    while (res.size() < 10) res.push_back("");
    return res;
}

// Template for a FLTK Picker window
class Picker : public Fl_Window {
public:
    Fl_Input *input;
    std::vector<Fl_Box*> boxes;
    int selected = 0;

    Picker() : Fl_Window(400,120,"Emoji Picker") {
        color(fl_rgb_color(18, 52, 86));             // dark background
        selection_color(fl_rgb_color(0,100,200));    // The selection highlight

        // Seach field 
        input = new Fl_Input(10,10,380,25);
        input->when(FL_WHEN_CHANGED);
        input->callback(cb_input, this);
        input->color(fl_rgb_color(30,30,30));
        input->textcolor(FL_WHITE);
        input->selection_color(fl_rgb_color(0,80,160));
        
        // The boxes that contains the emojies
        for (int i = 0; i < 10; i++) {
            auto b = new Fl_Box(10 + i*35, 50, 30, 30, "");
            b->box(FL_BORDER_BOX);
            b->color(fl_rgb_color(30,30,30));
            b->labelcolor(FL_WHITE);
            boxes.push_back(b);
        }

        end();
    }

    // Update resulst on update of search field 
    static void cb_input(Fl_Widget*, void *v) {
        ((Picker*)v)->update_results();
    }

    void update_results() {
        auto res = search(input->value());
        for (int i = 0; i < 10; i++) {
            if (i < (int)res.size())
                boxes[i]->copy_label(res[i].c_str());
            else
                boxes[i]->copy_label("");
        }
        selected = 0;
        highlight();
        redraw();
    }
    
    // Highlights the selected square
    void highlight() {
        for (int i = 0; i < 10; i++) {
            boxes[i]->color(i==selected ? fl_rgb_color(0,80,160) : fl_rgb_color(30,30,30));
        }
    }
    
    // Keyboard navigation
    int handle(int e) override {
        switch(e) {
            case FL_KEYDOWN:
                if (Fl::event_key() == FL_Right || Fl::event_key() == FL_Tab) {
                    selected = (selected+1)%10;
                    highlight(); redraw(); return 1;
                }
                if (Fl::event_key() == FL_Left) {
                    selected = (selected+9)%10;
                    highlight(); redraw(); return 1;
                }
                if (Fl::event_key() == FL_Enter) {
                    const char* txt = boxes[selected]->label();
                    if (txt && *txt) {
                        std::string emoji(txt);
                        std::string cmd = "echo -n '" + emoji + "' | xclip -selection clipboard";
                        (void)system(cmd.c_str()); // copy to clipboard
                    }
                    hide(); 
                    return 1;
                }
        }
        return Fl_Window::handle(e);
    }
};

int main() {
    load_db();
    Picker p;
    p.show();
    return Fl::run();
}

