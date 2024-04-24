#include <iostream>           // for stdio/stdout
#include <SFML/Audio.hpp>     // For audio
#include <SFML/Graphics.hpp>  // for graphics
#include <fstream>            // for file i/o
#include <vector>             // for dynamic lists/vectors
#include <sstream>            // for .split()
#include <cmath>              // for math
#include <unistd.h>           // for sleep()
using namespace std;          // shorthand

map<string, string> read_key_value(ifstream &stream, string delim);
vector<vector<string>> read_comma_pairs(ifstream &stream);
vector<string> split(string &s, char delim);
float linear_map(float num, float start1, float end1, float start2, float end2);
bool get_nth_bit(int num, int idx);
int get_bit_range(int num, int start, int stop);

int main() {

    // Parsing file
    ifstream osu_file;
    osu_file.open("beatmap1/ribbon room - mint tears (Shirasaka Koume) [extreme].osu");
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

    vector<vector<string>> hit_objects = read_comma_pairs(osu_file);

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

    // Preloading combo colours & number
    int starting_index = 0;
    int current_combo = 1;
    for (auto &hit_object : hit_objects) {
        if (get_nth_bit(stoi(hit_object[3]), 2) && !(get_nth_bit(stoi(hit_object[3]), 3))) {
            int index_jump = get_bit_range(stoi(hit_object[3]), 4, 6) + 1;
            starting_index = (starting_index + index_jump) % colours.size();
            current_combo = 1;
        }

        hit_object.push_back(to_string(current_combo));
        hit_object.push_back(colours[starting_index][0]);
        hit_object.push_back(colours[starting_index][1]);
        hit_object.push_back(colours[starting_index][2]);
        current_combo++;
    }

    // Adding some window settings
    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;  // can tweak this as needed


    // Adding a window
    sf::RenderWindow window(sf::VideoMode(512, 384), ".osu visualizer", sf::Style::Default, settings);

    // Background
    sf::Texture texture;
    if (!texture.loadFromFile("beatmap1/BG.jpg")) return 1;
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
    if (!music.openFromFile("beatmap1/audio.mp3")) return 1;
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
        if (delta_time > stoi(hit_objects[0][2])){
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
        vector<vector<string>> active_hit_objects;
        for (const auto &hit_object : hit_objects) {
            // If it's too early to draw anything
            if (delta_time < stoi(hit_object[2]) - PREEMPT) break;
            active_hit_objects.insert(active_hit_objects.begin(), hit_object);
        }

        // drawing hitcircles
        for (const auto &hit_object : active_hit_objects) {

            // Opacity of hitcircle and approach circle
            int opacity = linear_map(min(delta_time, stoi(hit_object[2]) - PREEMPT + FADE_IN), stoi(hit_object[2]) - PREEMPT, stoi(hit_object[2]) - PREEMPT + FADE_IN, 0, 255);

            // Drawing hitcircle
            sf::Color circ_colour(stoi(hit_object[hit_object.size() -3]), stoi(hit_object[hit_object.size() -2]), stoi(hit_object[hit_object.size() - 1]), opacity);

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
            hitcircle.setPosition(stoi(hit_object[0]) - RADIUS, stoi(hit_object[1]) - RADIUS);
            hitcircle.setColor(circ_colour);
            window.draw(hitcircle);

            sf::Sprite hitcircleoverlay;
            hitcircleoverlay.setTexture(hitcircleoverlay_texture);
            hitcircleoverlay.setScale(sf::Vector2f(RADIUS * 2 / hitcircleoverlay_texture.getSize().x, RADIUS * 2 / hitcircleoverlay_texture.getSize().y));
            hitcircleoverlay.setPosition(stoi(hit_object[0]) - RADIUS, stoi(hit_object[1]) - RADIUS);
            hitcircleoverlay.setColor(sf::Color(255, 255, 255, opacity));
            window.draw(hitcircleoverlay);

            // Drawing approach circle
            float scale_factor = linear_map(stoi(hit_object[2]) - delta_time, PREEMPT, 0, 5, 1);

            sf::Sprite approachcircle;
            approachcircle.setTexture(approachcircle_texture);
            approachcircle.setScale(sf::Vector2f(
                RADIUS * 2 / approachcircle_texture.getSize().x * scale_factor,
                RADIUS * 2 / approachcircle_texture.getSize().y * scale_factor
            ));
            approachcircle.setPosition(stoi(hit_object[0]) - RADIUS * scale_factor, stoi(hit_object[1]) - RADIUS * scale_factor);
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
