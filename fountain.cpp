#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>
#include <regex>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <csignal>


std::vector<int> get_cursors() {
  Display* display = XOpenDisplay(NULL);
  std::regex r("test_ptr\\d\\d\\d pointer");

  int num_devices;
  std::vector<int> cursors;
  XIDeviceInfo* devices = XIQueryDevice(display, XIAllDevices, &num_devices);
  for (int i = 0; i < num_devices; ++i) {
    XIDeviceInfo& dev = devices[i];
    if (std::regex_match(dev.name, r)) {
      cursors.push_back(dev.deviceid);
    }
  }
  return cursors;
}

void clear_cursors() {
  Display* display = XOpenDisplay(NULL);

  std::vector<int> cursors = get_cursors();
  for (int cursor : cursors) {
    XIRemoveMasterInfo remove;
    remove.type = XIRemoveMaster;
    remove.deviceid = cursor;
    remove.return_mode = XIAttachToMaster;
    remove.return_pointer = 2;
    remove.return_keyboard = 3;
    XIChangeHierarchy(display, (XIAnyHierarchyChangeInfo*)&remove, 1);
  }
  XSync(display, 0);
}

std::string cursor_name(int i) {
  std::ostringstream oss;
  oss << "test_ptr" << std::setfill('0') << std::setw(3) << i;
  return oss.str();
}

void make_cursors(int n) {
  Display* display = XOpenDisplay(NULL);
  for (int i = 0; i < n; ++i ) {
    XIAddMasterInfo add;
    std::string name = cursor_name(i);
    add.type = XIAddMaster;
    add.name = &name[0];
    add.send_core = 1;
    add.enable = 1;
    XIChangeHierarchy(display, (XIAnyHierarchyChangeInfo*)&add, 1);
  }
  XSync(display, 0);
}

void exit_handler(int sig) {
  clear_cursors();
  exit(0);
}

long random(long min, long max) {
  return min + (rand() % (max - min));
}

struct particle {
  float x, y;
  float vx, vy;
};
void run(std::vector<int> cursors) {
  Display* display = XOpenDisplay(NULL);
  const int W = XDisplayWidth(display, 0);
  const int H = XDisplayHeight(display, 0);
  Window root = XDefaultRootWindow(display);

  std::vector<particle> particles;
  for (int i = 0; i < cursors.size(); ++i) {
    particle p;
    p.x = 0;
    p.y = -1;
    particles.push_back(p);
  }

  while (true) {
    for (int i = 0; i < cursors.size(); ++i) {
      particle& p = particles[i];
      if (p.y < 0) {
        p.x = W / 2 + random(-30, 30);
        p.y = 0;
        p.vx = random(-5, 5);
        p.vy = random(30, 70);
      }
      p.x += p.vx;
      p.y += p.vy;
      p.vy -= 2;
      int x = p.x;
      int y = H - p.y;
      XIWarpPointer(display, cursors[i], None, root, 0, 0, 0, 0, x, y);
      XFlush(display);
    }
    usleep(16000);
  }
}

int main(int argc, char** argv) {
  struct sigaction sig_int_handler;
  sig_int_handler.sa_handler = exit_handler;
  sigemptyset(&sig_int_handler.sa_mask);
  sig_int_handler.sa_flags = 0;
  sigaction(SIGINT, &sig_int_handler, NULL);

  clear_cursors();
  make_cursors(60);
  run(get_cursors());
  return 0;
}
