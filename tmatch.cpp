#include <iostream>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define BUFFER_SIZE 16

using namespace std;


class State {
public:
  /// Constructor
  State() {}
  State(char *filename) {
    int ret;

    fd_ = open(filename, O_RDONLY);

    struct stat buf;
    ret = fstat(fd_, &buf);
    if (ret <= 0) {
      /// TBD
    }

    offset_ = buf.st_size;
    curp_ = offset_;

    readbuf();
  }

  /// Relative pos in local buf.
  size_t pos() {
    return curp_ - offset_;
  }

  /// Get one char at pos()
  char get_char() {
    return buf_[pos()];
  }

  /// Trim buffer.
  void trim() {
    buf_.resize(pos());
  }

  /// Move cursor backward.
  void move_p(size_t n) {
    curp_ -= n;
  }

  /// Read bytes from FD, and update the buffer.
  string readbuf() {
    char tmp[BUFFER_SIZE + 1] = {0};
    size_t nread;
    size_t n;

    // If the offset is smaller than BUFFER_SIZE, it can only read offset size.
    if (offset_ < BUFFER_SIZE) {
      n = offset_;
    } else {
      n = BUFFER_SIZE;
    }

    offset_ -= n;
    off_t off = lseek(fd_, offset_, SEEK_SET);
    if (off != offset_) {
      // TBD something wrong.
    }

    // Read n bytes.
    nread = read(fd_, (void *)tmp, n);
    tmp[nread] = '\0';

    // Update buffer [new data]+[old]
    buf_ = string(tmp) + buf_;

    return string(tmp);
  }

  /// Find the next '\n' in the buffer toward the beginning.
  /// Update buffer with readbuf() if it cannot find the one.
  /// Return true if '\n' is found and there is still data in the buffer
  /// Otherwise it hits the beginning of the file, return false.
  bool find_hol() {
    while (get_char() != '\n') {
      if (pos() == 0) {
        if (offset_ == 0)
          return false;

        readbuf();
      }
      move_p(1);
    }

    return true;
  }

  /// Search keyword in the buf and return vector of matched lines.
  vector<string> search(char *keyword, size_t max) {
    vector<string> v;
    char len = strlen(keyword);
    char needle = keyword[len - 1];

    while (v.size() < max) {
      if (pos() < len) {
        readbuf();
      }

      bool found = false;
      while (pos() > 0) {
        move_p(1);
        char c = get_char();

        // Find '\n' or the last letter of the keyword (needle).
        if (c == '\n') {
          trim();
        } else if (c == needle) {
          found = true;
          break;
        }

        if (pos() == 0) {
          if (offset_ == 0) {
            return v;
          }
          readbuf();
        }
      }

      if (pos() < len) {
        readbuf();
      }

      if (found) {
        size_t p = pos() - (len - 1);
        if (buf_.find(keyword, p, len) == p) {
          move_p(len - 1);

          if (find_hol()) {
            auto line = buf_.substr(pos() + 1);
            v.emplace_back(line);
          } else {
            auto line = buf_.substr(pos());
            v.emplace_back(line);
            return v;
          }

          trim();
        } else {
          move_p(1);
        }
      }
    }

    return v;
  }

private:
  /// File descriptor.
  int fd_;

  /// Buffer.
  string buf_;

  /// Current offset in the file.
  size_t offset_;

  /// Current search position.
  size_t curp_;
};

/// Print usage.
void
print_usage(char *prog) {
  cout << "Usage: " << prog << " <filename> <keyword>" << endl;
  exit(1);
}

/// Main function.
int
main(int argc, char **argv)
{
  if (argc < 3) {
    print_usage(argv[0]);
  }

  char *filename = argv[1];
  char *keyword = argv[2];
  State *state = new State(filename);

  auto v = state->search(keyword, 5);

  for (auto s: v) {
    cout << s << endl;
  }
}

