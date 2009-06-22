#pragma once

class Archive;
struct Vect2i{

    enum ID_init{ ID };
    enum ZERO_init{ ZERO };
    int x;
    int y;

    Vect2i()
    {
    }
    Vect2i(int _x, int _y){
        set(_x, _y);
    }
    Vect2i(ID_init){
        set(1, 0);
    }
    Vect2i(ZERO_init){
        set(0, 0);
    }
    void set(int _x, int _y){
        x = _x;
        y = _y;
    }
    bool operator==(const Vect2i& rhs) const{
        return x == rhs.x && y == rhs.y;
    }
    bool operator!=(const Vect2i& rhs) const{
        return x != rhs.x || y != rhs.y;
    }
    Vect2i operator-() const{ return Vect2i(-x, -y); }
    Vect2i operator-(const Vect2i& rhs) const{ return Vect2i(x - rhs.x, y - rhs.y); }
    Vect2i operator+(const Vect2i& rhs) const{ return Vect2i(x + rhs.x, y + rhs.y); }
    Vect2i operator*(const Vect2i& rhs) const{ return Vect2i(x * rhs.x, y * rhs.y); }
    Vect2i operator*(int rhs) const{ return Vect2i(x * rhs, y * rhs); }
    Vect2i operator/(const Vect2i& rhs) const{ return Vect2i(x / rhs.x, y / rhs.y); }
    Vect2i operator/(int rhs) const{ return Vect2i(x / rhs, y / rhs); }
	Vect2i& operator+=(const Vect2i& rhs)
	{
		x += rhs.x;
		y += rhs.y;
		return *this;
	}
	Vect2i& operator-=(const Vect2i& rhs)
	{
		x -= rhs.x;
		y -= rhs.y;
		return *this;
	}
	int norm2() const{ return x * x + y * y; }

    void serialize( Archive& ar );
};


