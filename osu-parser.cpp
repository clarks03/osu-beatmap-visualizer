#include <iostream>           // for stdio/stdout
#include <SFML/Audio.hpp>     // For audio
#include <SFML/Graphics.hpp>  // for graphics
#include <fstream>            // for file i/o
#include <vector>             // for dynamic lists/vectors
#include <sstream>            // for .split()
#include <cmath>              // for math
#include <unistd.h>           // for sleep()
#include <filesystem>         // for filesystem shenanigans
#include <zip.h>              // for unzipping .osz

using namespace std;          // shorthand
namespace fs = std::__fs::filesystem;

// Creating a class for hit objects

class Colour {
    public:
        int r;
        int g;
        int b;
};

class HitObject {
    public:
        int x;
        int y;
        int time;
        int type;
        int hitsound;
        vector<string> object_params;
        vector<string> hit_sample;
        int combo;
        Colour colour;
};

map<string, string> read_key_value(ifstream &stream, string delim);
vector<vector<string>> read_comma_pairs(ifstream &stream);
vector<string> split(string &s, char delim);
float linear_map(float num, float start1, float end1, float start2, float end2);
bool get_nth_bit(int num, int idx);
int get_bit_range(int num, int start, int stop);
int extract_zip(const string &compressed, const string &destination);

int main() {

    // Code for checking if any maps have been loaded
    // (Seeing if there exists a Songs/ directory here)

    if (fs::is_directory("Songs")) {
        cout << "Songs directory found" << endl;
    } else {
        cout << "No Songs directory found yet... presumably first install" << endl;
    }

    // Now checking for .osz files in the current directory
    for (const auto &entry : fs::directory_iterator(".")) {
        if (entry.is_regular_file() && entry.path().extension() == ".osz") {

            // Check if the Songs directory has been created yet... if not, create it
            if (!fs::is_directory("Songs")) fs::create_directory("Songs");

            // Also checking if this song has been added already
            if (fs::is_directory("Songs/" + entry.path().stem().string())) continue;

            // Create a dedicated folder for this song
            fs::create_directory("Songs/" + entry.path().stem().string());

            // Debugging purposes... Useful information, you know!
            cout << "Loading " << entry.path().string() << endl;

            // Unzipping the .osz file
            // THIS IS A REALLY BAD WAY TO DO THIS... EVENTUALLY I'LL USE SOMETHING MORE ROBUST
            if (extract_zip(entry.path().string(), "Songs/" + entry.path().stem().string()) != 0) {
                cout << "Failed to extract " << entry.path().string() << endl;
                return 1;
            }

            // now I can delete the archive
            fs::remove(entry.path().string());
        }
    }
    

    // If, by this point, the Songs/ directory still doesn't exist,
    // it means the user has not downloaded any songs yet,
    // and we can exit the program.
    if (!fs::is_directory("Songs")) {
        cout << "No songs found... exiting program" << endl;
        return 1;
    }

    cout << "\n\n" << endl;

    // Now, we can ask the user which song they want to visualize
    cout << "Which song would you like to visualize?" << endl;
    int i = 1;
    for (const auto &entry : fs::directory_iterator("Songs")) {
        cout << "\t" << i << ") " << entry.path().filename().string() << endl;
        i++;
    }
    int idx;
    cin >> idx;

    cout << "You selected: " << idx << endl;
    i = 1;
    string song_path;
    for (const auto &entry : fs::directory_iterator("Songs")) {
        if (i == idx) {
            song_path = entry.path().filename().string();
            break;
        }
        i++;
    }

    cout << "\nWhich difficulty would you like to visualize?" << endl;
    i = 1;
    for (const auto &entry : fs::directory_iterator("Songs/" + song_path)) {
        if (entry.path().extension() != ".osu") continue;
        // only get the difficulty name
        // cout << entry.path().filename().string().find(']') << endl;
        string file_name = entry.path().filename().string();
        string difficulty_name = file_name.substr(file_name.find("[") + 1, file_name.find(']') - file_name.find('[') - 1);
        cout << "\t" << i << ") " << difficulty_name << endl;
        i++;
    }
    cin >> idx;

    i = 1;
    string difficulty_name;
    for (const auto &entry : fs::directory_iterator("Songs/" + song_path)) {
        if (entry.path().extension() != ".osu") continue;
        if (i == idx) {
            difficulty_name = entry.path().filename().string();
            break;
        }
        i++;
    }
    cout << difficulty_name << endl;

    // Parsing file
    ifstream osu_file;
    osu_file.open("Songs/" + song_path + "/" + difficulty_name);
    map<string, string> general;

    string line;
    // Getting the format
    getline(osu_file, line);
    // Clearing a blank space
    getline(osu_file, line);
    // Removing the [General] tag
    getline(osu_file, line);

    // Reading in values
    general = read_key_value(osu_file, ": ");
    map<string, string> editor = read_key_value(osu_file, ": ");
    map<string, string> metadata = read_key_value(osu_file, ":");
    map<string, string> difficulty = read_key_value(osu_file, ":");
    vector<vector<string>> events = read_comma_pairs(osu_file);
    vector<vector<string>> timing_points = read_comma_pairs(osu_file);

    // Colours is a bit finnicky... need to do this instead
    map<string, string> colours_map = read_key_value(osu_file, " : ");
    vector<vector<string>> colours;
    for (auto &colour : colours_map) {
        vector<string> rgb = split(colour.second, ',');
        colours.push_back(rgb);
    }

    vector<vector<string>> hit_objects_str = read_comma_pairs(osu_file);

    // converting the 2d array of strings to a vector of HitObject objects
    // Also initializing combo numbers and colours here
    vector<HitObject> hit_objects;
    int starting_index = 0;
    int current_combo = 1;
    for (auto &hit_object : hit_objects_str) {
        HitObject obj;
        obj.x = stoi(hit_object[0]);
        obj.y = stoi(hit_object[1]);
        obj.time = stoi(hit_object[2]);
        obj.type = stoi(hit_object[3]);
        obj.hitsound = stoi(hit_object[4]);
        obj.object_params = split(hit_object[5], ',');

        if (get_nth_bit(obj.type, 2) && !(get_nth_bit(obj.type, 3))) {
            int index_jump = get_bit_range(obj.type, 4, 6) + 1;
            starting_index = (starting_index + index_jump) % colours.size();
            current_combo = 1;
        }
        obj.combo = current_combo;
        obj.colour = Colour();
        obj.colour.r = stoi(colours[starting_index][0]);
        obj.colour.g = stoi(colours[starting_index][1]);
        obj.colour.b = stoi(colours[starting_index][2]);
        current_combo++;
        if (hit_object.size() < 7) obj.hit_sample = vector<string>{"0:0:0:0"};
        else obj.hit_sample = split(hit_object[6], ',');
        hit_objects.push_back(obj);
    }

    // Canon approach rate settings
    const float APPROACH_RATE = stof(difficulty.find("ApproachRate")->second);
    float PREEMPT, FADE_IN;

    if (APPROACH_RATE > 5) {
        PREEMPT = 1200 + 600 * (5 - APPROACH_RATE) / 5;
        FADE_IN = 800 + 400 * (5 - APPROACH_RATE) / 5;
    } else if (APPROACH_RATE < 5) {
        PREEMPT = 1200 - 750 * (APPROACH_RATE - 5) / 5;
        FADE_IN = 800 - 500 * (APPROACH_RATE - 5) / 5;
    } else {
        PREEMPT = 1200;
        FADE_IN = 800;
    }

    // Canon circle size settings
    const float CIRCLE_SIZE = stof(difficulty.find("CircleSize")->second);
    const float RADIUS = 54.4 - 4.48 * CIRCLE_SIZE;

    // Adding some window settings
    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;  // can tweak this as needed


    // Adding a window
    sf::RenderWindow window(sf::VideoMode(512, 384), ".osu visualizer", sf::Style::Default, settings);

    // Background
    // This is tedious, but to find the background image we need to iterate over all events
    string bg_path;
    for (const auto &event : events) {
        if (event[0] == "0" && event[1] == "0") {
            bg_path = event[2].substr(1, event[2].length() - 2);
            break;
        }
    }

    sf::Texture texture;
    if (!texture.loadFromFile("Songs/" + song_path + "/" + bg_path)) return 1;
    texture.setSmooth(true);
    sf::Sprite sprite;
    sprite.setTexture(texture);
    sprite.setScale(sf::Vector2f(512.f / texture.getSize().x, 384.f / texture.getSize().y));

    // Hitcircle & Hitcircleoverlay
    sf::Texture hitcircle_texture;
    if (!hitcircle_texture.loadFromFile("hitcircle.png")) return 1;
    hitcircle_texture.setSmooth(true);

    sf::Texture hitcircleoverlay_texture;
    if (!hitcircleoverlay_texture.loadFromFile("hitcircleoverlay.png")) return 1;
    hitcircleoverlay_texture.setSmooth(true);

    // Approachcircle
    sf::Texture approachcircle_texture;
    if (!approachcircle_texture.loadFromFile("approachcircle.png")) return 1;
    approachcircle_texture.setSmooth(true);

    // Hitsound
    sf::SoundBuffer hitsound_buffer;
    if (!hitsound_buffer.loadFromFile("untitled.mp3")) return 1;

    sf::Sound hitsound;
    hitsound.setBuffer(hitsound_buffer);
    hitsound.setVolume(20);

    // Music
    sf::Music music;
    // Need everything except for the last (newline) character to avoid PROBLEMS!!!!!!!!
    string audio_filename = general.find("AudioFilename")->second.substr(0, general.find("AudioFilename")->second.length() - 1);
    if (!music.openFromFile("Songs/" + song_path + "/" + audio_filename)) return 1;
    music.setVolume(10);
    music.play();

    // Timer (for hitcircle stuff--VERY IMPORTANT!!)
    sf::Clock clock;
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }

        // current time
        float delta_time = clock.getElapsedTime().asMilliseconds();

        // clearing window
        window.clear();
        // drawing background
        window.draw(sprite);

        // removing hitcircle once it should no longer be displayed on screen
        if (delta_time > hit_objects[0].time) {
            hit_objects.erase(hit_objects.begin());
            hitsound.play();
        }

        // If, at this point, we've run out of circles, we should stop for 2 seconds
        // before breaking
        if (hit_objects.size() == 0) {
            sleep(2);
            window.close();
        }

        // Getting active objects
        // vector<vector<string>> active_hit_objects;
        vector<HitObject> active_hit_objects;
        for (const auto &hit_object : hit_objects) {
            // If it's too early to draw anything
            if (delta_time < hit_object.time - PREEMPT) break;
            active_hit_objects.insert(active_hit_objects.begin(), hit_object);
        }

        // drawing hitcircles
        for (const auto hit_object : active_hit_objects) {

            // Opacity of hitcircle and approach circle
            int opacity = linear_map(min(delta_time, hit_object.time - PREEMPT + FADE_IN), hit_object.time - PREEMPT, hit_object.time - PREEMPT + FADE_IN, 0, 255);

            // Drawing hitcircle
            sf::Color circ_colour(hit_object.colour.r, hit_object.colour.g, hit_object.colour.b, opacity);

            // Old code for drawing a plain circle

            /*
            sf::CircleShape circ((float) RADIUS);

            sf::Color circ_colour(stoi(hit_object[hit_object.size() -3]), stoi(hit_object[hit_object.size() -2]), stoi(hit_object[hit_object.size() - 1]), opacity);
            circ.setFillColor(circ_colour);
            circ.setPosition(stoi(hit_object[0]) - RADIUS, stoi(hit_object[1]) - RADIUS);
            window.draw(circ);
            */

            // New (AWESOME) hitcircle code
            sf::Sprite hitcircle;
            hitcircle.setTexture(hitcircle_texture);
            hitcircle.setScale(sf::Vector2f(RADIUS * 2 / hitcircle_texture.getSize().x, RADIUS * 2 / hitcircle_texture.getSize().y));
            hitcircle.setPosition(hit_object.x - RADIUS, hit_object.y - RADIUS);
            hitcircle.setColor(circ_colour);
            window.draw(hitcircle);

            sf::Sprite hitcircleoverlay;
            hitcircleoverlay.setTexture(hitcircleoverlay_texture);
            hitcircleoverlay.setScale(sf::Vector2f(RADIUS * 2 / hitcircleoverlay_texture.getSize().x, RADIUS * 2 / hitcircleoverlay_texture.getSize().y));
            hitcircleoverlay.setPosition(hit_object.x - RADIUS, hit_object.y - RADIUS);
            hitcircleoverlay.setColor(sf::Color(255, 255, 255, opacity));
            window.draw(hitcircleoverlay);

            // Drawing approach circle
            float scale_factor = linear_map(hit_object.time - delta_time, PREEMPT, 0, 5, 1);

            sf::Sprite approachcircle;
            approachcircle.setTexture(approachcircle_texture);
            approachcircle.setScale(sf::Vector2f(
                RADIUS * 2 / approachcircle_texture.getSize().x * scale_factor,
                RADIUS * 2 / approachcircle_texture.getSize().y * scale_factor
            ));
            approachcircle.setPosition(hit_object.x - RADIUS * scale_factor, hit_object.y - RADIUS * scale_factor);
            approachcircle.setColor(circ_colour);
            window.draw(approachcircle);


            // Old code for drawing a plain circle

            /*
            // First, we need to map the size
            sf::CircleShape approachcirc(RADIUS * scale_factor);
            // Transparent body, opaque border
            approachcirc.setFillColor(sf::Color(0, 0, 0, 0));;
            approachcirc.setOutlineColor(circ_colour);
            approachcirc.setOutlineThickness(scale_factor);
            approachcirc.setPosition(stoi(hit_object[0]) - (RADIUS * scale_factor), stoi(hit_object[1]) - (RADIUS * scale_factor));
            window.draw(approachcirc);
            */

        }


        // displaying window stuff (THIS SHOULD BE LAST)
        window.display();

    }

    return 0;
}

map<string, string> read_key_value(ifstream &stream, string delim) {
    map<string, string> result;
    string line;
    while (getline(stream, line)) {
        if (line.empty() || line == "\r") continue;
        if (line[0] == '[') return result;
        size_t pos = line.find(delim);
        if (pos != string::npos) {
            string key = line.substr(0, pos);
            string value = line.substr(pos + delim.length());
            result[key] = value;
        }
    }
    return result;
}

vector<vector<string>> read_comma_pairs(ifstream &stream) {
    vector<vector<string>> data;
    string line;
    while (getline(stream, line)) {
        if (line.empty() || line == "\r") continue;
        if (line[0] == '[') return data;
        if (line.substr(0, 2) == "//") continue;
        vector<string> events = split(line, ',');
        data.push_back(events);
    }
    return data;
}

vector<string> split (string &s, char delim) {
    vector<string> tokens;
    size_t start = 0, end = 0;
    while ((end = s.find(delim, start)) != string::npos) {
        if (end != start) {
            tokens.push_back(s.substr(start, end - start));
        }
        start = end + 1;
    }
    if (start < s.length()) {
        tokens.push_back(s.substr(start));
    }
    return tokens;
}

float linear_map(float num, float start1, float end1, float start2, float end2) {
    float proportion = (num - start1) / (end1 - start1);
    float mapped = start2 + proportion * (end2 - start2);
    return mapped;
}

bool get_nth_bit(int num, int idx) {
    return 1 == ((num >> idx) & 1);
}

int get_bit_range(int num, int start, int stop) {
    int waste = 31 - stop;
    return (num << waste) >> (waste + start);
}

// I LOVE CHATGPT!!!!!!!
int extract_zip(const string &compressed, const string &destination) {
    struct zip *archive = zip_open(compressed.c_str(), 0, nullptr);
    if (!archive) {
        cerr << "Failed to open ZIP file: " << zip_strerror(archive) << endl;
        return -1;
    }

    int num_entries = zip_get_num_entries(archive, 0);

    for (int i = 0; i < num_entries; i++) {
        struct zip_stat stat;
        zip_stat_init(&stat);
        zip_stat_index(archive, i, 0, &stat);

        // Allocate memory for the new data
        char *data = new char[stat.size];

        // Read the entry data
        zip_file_t *file = zip_fopen_index(archive, i, 0);
        if (!file) {
            cerr << "Failed to open file in ZIP: " << zip_strerror(archive) << endl;
            zip_close(archive);
            delete[] data;
            return -1;
        }

        zip_fread(file, data, stat.size);
        zip_fclose(file);

        string entry_path = destination + "/" + stat.name;
        ofstream out_file(entry_path, ios::binary);
        out_file.write(data, stat.size);
        out_file.close();

        delete[] data;
    }

    zip_close(archive);

    return 0;
}