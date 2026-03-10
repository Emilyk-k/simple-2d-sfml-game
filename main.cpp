/*
                                                    CONTROLS:
                                                    w, a, s, d - move
                                                    hold LMB - shoot
                                                    ~ - show console

*/
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <limits>
#include <windows.h>
#include <math.h>

typedef std::vector<sf::IntRect> IntRectSeq;

class TextLine :public sf::Text{
public:
    TextLine(const std::string &fontname, const std::string &string, const int &size, const sf::Color &color, const float &originx, const float &originy){
        if (!font.loadFromFile(fontname))
        {
            std::cerr << "Font file not found" << std::endl;
        }
        font.loadFromFile(fontname);
        setFont(font);
        setString(string);
        setCharacterSize(size);
        setFillColor(color);
        setOrigin(originx, originy);
    }
    ~TextLine(){}
protected:
    sf::Font font;
};

class Score :public TextLine{
public:
    using TextLine::TextLine;
    void Up(const int &amount){
        ScoreNum += amount;
        std::string str = "Score: " + std::to_string(ScoreNum);
        setString(str);
    }
    ~Score(){}
private:
    int ScoreNum = 0;
};

class AnimatedSprite :public sf::Sprite{
public:
    AnimatedSprite(const std::string& file, bool tiles = false){
        if (!texture.loadFromFile(file)){
            std::cerr << "Texture file missing or corrupted" << std::endl;
        }
        texture.loadFromFile(file);
        if (tiles)
            texture.setRepeated(true);
        setTexture(texture);
    }
    void setAnimation(int fps = 0, const IntRectSeq &rectSeq = IntRectSeq()){
        this->fps = fps;
        if (fps <= 0)
            this->rectSeq.clear();
        else
            this->rectSeq = rectSeq;
        this->frame = 0;
        dt = std::numeric_limits<double>::max();
    }
    void step(const sf::Time& elapsed){
        if (!rectSeq.empty() && fps > 0){
            if (dt > 1./fps){
                if (frame <= 0 || frame >= (int)rectSeq.size())
                    frame  = 0;
                setTextureRect(rectSeq[frame]);
                dt = 0;
                ++frame;
            }
            dt += elapsed.asSeconds();
        }
    }
    void faceTheMouse(const sf::RenderWindow &window){
        const float pi = 3.14159265;
        sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
        sf::Vector2f spritePos = getPosition();

        float dx = spritePos.x - mousePos.x;
        float dy = spritePos.y - mousePos.y;
        float rotation = (atan2(dy, dx)) * 180/pi + 180;
        setRotation(rotation);
    }
    ~AnimatedSprite(){}

private:
    sf::Texture texture;
    int fps = 0, frame = 0;
    IntRectSeq rectSeq;
    double dt = 0;
};

class Projectile :public AnimatedSprite{
public:
    using AnimatedSprite::AnimatedSprite;

    void setArrSpd(const sf::Vector2f &v){
        this->arrSpd = v;
    }

    sf::Vector2f getArrSpd(){
        return this->arrSpd;
    }

    ~Projectile(){}
private:
    sf::Vector2f arrSpd = sf::Vector2f(0.f, 0.f);
};

void hideConsole(){
    HWND console = GetConsoleWindow();
    ShowWindow( console, SW_HIDE );
}

void showConsole(){
    HWND console = GetConsoleWindow();
    ShowWindow(console, SW_SHOW);
}

bool isConsole()
{
    return ::IsWindowVisible(::GetConsoleWindow());
}

void consoleSwitch(const sf::Event &event){
    if ((event.type == sf::Event::KeyPressed) && (event.key.code == sf::Keyboard::Key::Tilde)){
        if(!isConsole()){
            showConsole();
        } else{
            hideConsole();
        }
    }
}

int main(){

    hideConsole();
    sf::RenderWindow window(sf::VideoMode(800, 600), "Game");

    //objects and variables

    AnimatedSprite grass("grass.png", true);
    grass.setTextureRect(sf::IntRect(0, 0, window.getSize().x, window.getSize().y));
    AnimatedSprite hero("characterfixed.png");
    AnimatedSprite weaponArm("weaponarm.png");
    Score score("unispace bd.ttf", "Score: 0", 24, sf::Color::White, -50, -50);
    AnimatedSprite target("target.png");
    Projectile arrow("arrow.png");

    const IntRectSeq heroRunning ={
        sf::IntRect(150, 0, 37, 37),
        sf::IntRect(200, 0, 37, 37),
        sf::IntRect(250, 0, 37, 37),
        sf::IntRect(300, 0, 37, 37),
        sf::IntRect(350, 0, 37, 37),
        sf::IntRect(400, 0, 37, 37),
    };
    const IntRectSeq heroStanding ={
        sf::IntRect(0, 0, 37, 37),
        sf::IntRect(50, 0, 37, 37),
        sf::IntRect(100, 0, 37, 37),
    };
    const IntRectSeq bowShot ={
        sf::IntRect(0, 0, 16, 18),
        sf::IntRect(18, 0, 17, 18),
        sf::IntRect(36, 0, 18, 18),
        sf::IntRect(56, 0, 19, 18),
    };
    const IntRectSeq bowHeld ={
        sf::IntRect(0, 0, 16, 18),
    };

    bool isFacingL = false;
    const float heroScale = 2.f, heroX = 25.f, heroY = 18.5f, bowX = 5.5f, bowY = 10.5f, arrX = 6.f, arrY = 1.5f, arrSpdMax = 0.135f;
    const int heroFPS = 7, heroSpeed = 200, bowFPS = 4, winX = 800, winY = 600;
    enum class Action {Init, Standing, RunLeft, RunRight} Action = Action::Init;
    enum class bowAction {Init, Idle, Shot} bowAction = bowAction::Init;
    const sf::Vector2f bowStd(-5.f, -2.f), bowRLeft(-7.f, 1.f), bowRRight(6.5f, 0.f);
    sf::Vector2f mousePos(0.f, 0.f), heroPos(0.f, 0.f), arrDir(0.f, 0.f), arrDirNormal(0.f, 0.f);
    const sf::Int64 AtkSpd = 1000000, TargetSpawn = 1500000;
    sf::Int64 BowCharge = 0, TargetTime = 0;

    //objects setup

    hero.setOrigin(heroX, heroY);
    hero.setPosition(window.getSize().x/2, window.getSize().y/2);
    weaponArm.setOrigin(bowX, bowY);
    target.setScale(heroScale, heroScale);
    target.setOrigin(heroX, heroY);
    target.setAnimation(heroFPS, heroStanding);
    arrow.setOrigin(arrX, arrY);
    arrow.setScale(heroScale, heroScale);

    std::vector<const sf::Drawable*> shapes = {&grass, &hero, &score, &weaponArm};
    std::vector<AnimatedSprite> targets;
    std::vector<Projectile> arrows;
    for (int i = 0; i < 5; ++i){
        target.setPosition(rand() % winX, rand() % winY);
        targets.emplace_back(target);
        }

    // run the program as long as the window is open
    sf::Clock clock;
    while (window.isOpen()){
        const sf::Time elapsed = clock.restart();

        // check all the window's events that were triggered since the last iteration of the loop
        sf::Event event;
        while (window.pollEvent(event)){
            consoleSwitch(event);
            // window closed
            if (event.type == sf::Event::Closed || sf::Keyboard::isKeyPressed(sf::Keyboard::Escape))
                window.close();
        }

        bool key = false, key2 = false;

        //PLAYER

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) && hero.getPosition().y-24 > 0){
            hero.move(0, -elapsed.asSeconds()*heroSpeed);
            if(isFacingL == false){
            weaponArm.setPosition(hero.getPosition() + bowRRight);
            } else weaponArm.setPosition(hero.getPosition() + bowRLeft);
            if (Action != Action::RunRight && Action != Action::RunLeft){
                Action  = Action::RunRight;
                hero.setScale(heroScale, heroScale);
                hero.setAnimation(heroFPS, heroRunning);
            }
            key = true;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) && hero.getPosition().y+36 < 600){
            hero.move(0, +elapsed.asSeconds()*heroSpeed);
            if(isFacingL == false){
            weaponArm.setPosition(hero.getPosition() + bowRRight);
            } else weaponArm.setPosition(hero.getPosition() + bowRLeft);
            if (Action != Action::RunRight && Action != Action::RunLeft){
                Action  = Action::RunRight;
                hero.setScale(heroScale, heroScale);
                hero.setAnimation(heroFPS, heroRunning);
            }
            key = true;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) && hero.getPosition().x+18 < 800){
            hero.move(+elapsed.asSeconds()*heroSpeed, 0);
            if(isFacingL == false){
            weaponArm.setPosition(hero.getPosition() + bowRRight);
            } else weaponArm.setPosition( hero.getPosition() + bowRLeft);
            if (Action != Action::RunRight){
                Action  = Action::RunRight;
                hero.setScale(heroScale, heroScale);
                hero.setAnimation(heroFPS, heroRunning);
            }
            key = true;
            isFacingL = false;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) && hero.getPosition().x-20 > 0){
            hero.move(-elapsed.asSeconds()*heroSpeed, 0);
            weaponArm.setPosition(hero.getPosition() + bowRLeft);
            if (Action != Action::RunLeft){
                Action  = Action::RunLeft;
                hero.setScale(-heroScale, heroScale);
                hero.setAnimation(heroFPS, heroRunning);
            }
            key = true;
            isFacingL = true;
        }
        if (!key){
            if (Action != Action::Standing){
                Action  = Action::Standing;
                weaponArm.setPosition(hero.getPosition() + bowStd);
                hero.setScale(heroScale, heroScale);
                hero.setAnimation(heroFPS, heroStanding);
            }
            isFacingL = false;
        }

        //MOVING ARM

        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)){
            if (bowAction != bowAction::Shot){
                bowAction = bowAction::Shot;
                weaponArm.setScale(heroScale, heroScale);
                weaponArm.setAnimation(bowFPS, bowShot);
            }
            key2 = true;
        }
        weaponArm.faceTheMouse(window);

        if (!key2){
            if (bowAction != bowAction::Idle){
                bowAction = bowAction::Idle;
                weaponArm.setScale(heroScale, heroScale);
                weaponArm.setAnimation(bowFPS, bowHeld);
            }
        }

        //ARROWS

        heroPos = sf::Vector2f(hero.getPosition().x, hero.getPosition().y);
        mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
        arrDir = mousePos - heroPos;
        arrDirNormal = arrDir / float(sqrt(pow(arrDir.x, 2) + pow(arrDir.y, 2)));

        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)){
            arrow.setArrSpd(arrDirNormal * arrSpdMax);
            BowCharge += elapsed.asMicroseconds();
            if(BowCharge >= AtkSpd){
                arrow.setPosition(hero.getPosition());
                arrow.faceTheMouse(window);
                arrows.emplace_back(arrow);
                BowCharge = 0;
            }
        }
        if (!sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)){
            BowCharge = 0;
        }

        for(size_t i = 0; i < arrows.size(); ++i){
            arrows[i].move(arrows[i].getArrSpd());
            auto x = arrows[i].getPosition().x;
            auto y = arrows[i].getPosition().y;
            if(x < 0 || x > winX || y < 0 || y > winY){
                arrows.erase(arrows.begin() + i);
            }
        }

        //TARGETS

        TargetTime += elapsed.asMicroseconds();

        if(TargetTime >= TargetSpawn){
            target.setPosition(rand() % winX, rand() % winY);
            targets.emplace_back(target);
            TargetTime = 0;
        }

        for(size_t i = 0; i < targets.size(); ++i){
            if(targets[i].getPosition().y > window.getSize().y || targets[i].getPosition().x > window.getSize().x){
                targets.erase(targets.begin() + i);
            }
        }

        //HIT DETECTION

        for(size_t i = 0; i < arrows.size(); ++i){
            for(size_t j = 0; j < targets.size(); ++j){
                if(arrows[i].getGlobalBounds().intersects(targets[j].getGlobalBounds())){
                    arrows.erase(arrows.begin() + i);
                    targets.erase(targets.begin() + j);
                    score.Up(1);
                }
            }
        }

        //ANIMATION TRIGGERS

        weaponArm.step(elapsed);
        hero.step(elapsed);

        // clear the window with black color
        window.clear(sf::Color::Black);

        // draw everything here...
        for (auto &s: shapes)
            window.draw(*s);
        for(auto &t: targets){
            t.step(elapsed);
            window.draw(t);
        }
        for(auto &a: arrows){
            arrow.faceTheMouse(window);
            window.draw(a);
        }
        // end the current frame
        window.display();
    }

    return 0;
}
