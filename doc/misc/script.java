package baf.sci;

import java.awt.*;
import java.io.*;
import java.util.*;

class ScriptBlock {
  int start;
  byte buf[];

  public ScriptBlock(byte buf[], int address) {
    this.buf = buf;
    this.start = address;
  }

  public short getInt(int i) {
    short lo = (short)(0xff & (short)buf[i]);
    short hi = (short)buf[i+1];
    return (short)((hi<<8) | lo);
  }

  public short type() {
    return getInt(start+0);
  }

  public short len() {
    return getInt(start+2);
  }

  public boolean contains(int i) {
    return (i>=start && i<start+len());
  }

  private String opcodes[] = {
    "bnot", "add", "sub", "mul",
    "div", "mod", "shr", "shl",
    "xor", "and", "or", "neg",
    "not", "eq?", "ne?", "gt?",
    "ge?", "lt?", "le?", "ugt?",
    "uge?", "ult?", "ule?", "bt",
    "bnt", "jmp", "ldi", "push",
    "pushi", "toss", "dup", "link",
    "call", "callk", "callb", "calle",
    "ret", "send", "", "",
    "class", "", "self", "super",
    "&rest", "lea", "selfID", "",
    "pprev", "pToa", "aTop", "pTos",
    "sTop", "ipToa", "dpToa", "ipTos",
    "dpTos", "lofsa", "lofss", "push0",
    "push1", "push2", "pushSelf", "",
    "lag", "lal", "lat", "lap",
    "lsg", "lsl", "lst", "lsp",
    "lagi", "lali", "lati", "lapi",
    "lsgi", "lsli", "lsti", "lspi",
    "sag", "sal", "sat", "sap",
    "ssg", "ssl", "sst", "ssp",
    "sagi", "sali", "sati", "sapi",
    "ssgi", "ssli", "ssti", "sspi",
    "+ag", "+al", "+at", "+ap",
    "+sg", "+sl", "+st", "+sp",
    "+agi", "+ali", "+ati", "+api",
    "+sgi", "+sli", "+sti", "+spi",
    "-ag", "-al", "-at", "-ap",
    "-sg", "-sl", "-st", "-sp",
    "-agi", "-ali", "-ati", "-api",
    "-sgi", "-sli", "-sti", "-spi"
  };

  /*
   * 0 means no args.
   * 1 means one arg, int if eve,  byte if odd
   * 2 means special
    */
  private byte opargs[] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 2,
    2, 2, 1, 0, 1, 0, 0, 1,
    2, 2, 2, 2, 2, 2, 0, 0,
    1, 0, 2, 2, 1, 1, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1,
    1, 2, 2, 0, 0, 0, 0, 0,

    1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,

  };
    
  private String addstr(int i) {
    String s = Integer.toString(0xffff & i, 16).toUpperCase();
    while (s.length() < 4) s = "0"+s;
    return s;
  }

  private String bytestr(byte i) {
    String s = Integer.toString(0xff & (int)i, 16).toUpperCase();
    while (s.length() < 2) s = "0"+s;
    return s;
  }

  private int nextarg(TextArea t, int opcode, int i) {
    t.appendText(" ");
    if ((opcode & 1) == 0) {
      t.appendText(Integer.toString(getInt(i), 16).toUpperCase());
      i += 2;
    } else {
      t.appendText(Integer.toString(buf[i], 16).toUpperCase());
      i++;
    }
    return i;
  }

  public String typestr[] = {
    "Exit code",
    "Object",
    "Code",
    "Type 3",
    "Type 4",
    "String buffer",
    "Type 6",
    "Type 7",
    "Pointer table",    
    "Type 9",
    "Type 10"
  };

  public void display(TextArea t) {
    t.appendText(addstr(start));
    t.appendText(": ");
    t.appendText(typestr[type()]);
    int i=start + 4;
    int end = start + len();
    switch (type()) {
      case 2: { // code
        while (i < end) {
          t.appendText("\n");
          t.appendText(addstr(i));
          t.appendText("   ");
          int opcode = 0xff & (int)buf[i++];          
          t.appendText(opcodes[opcode/2]);
          switch (opargs[opcode/2]) {
            case 0: break; // no args
            case 1: {
              i = nextarg(t, opcode, i);
            } break;
            case 2: { // specials
              switch (opcode & 0xfe) {
                case 0x2e: // bt
                case 0x30: // bnt
                case 0x32: // jmp
                case 0x72: // lofsa
                case 0x74: // lofss
                {
                  short dest = getInt(i);
                  i += 2;
                  dest += i;
                  t.appendText(" ");
                  t.appendText(addstr(dest));
                } break;
                case 0x40: {
                  i = nextarg(t, opcode, i);
                  i = nextarg(t, 1, i);
                }
                case 0x42:
                case 0x44: {
                  i = nextarg(t, opcode, i);
                  i = nextarg(t, 1, i);
                } break;
                case 0x46: {
                  i = nextarg(t, opcode, i);
                  i = nextarg(t, opcode, i);
                  i = nextarg(t, 1, i);
                } break;
                case 0x48: { // ret
                  t.appendText("\n");
                } break;
                case 0x4a:
                case 0x54: { // always one byte
                  i = nextarg(t, 1, i);
                } break;
                case 0x56: {
                  i = nextarg(t, opcode, i);
                  i = nextarg(t, 1, i);
                }
              }
            } break;
          }
        }
      }
      case 5: { // text block
        byte b = 0;
        while (i < end) {
          if (b == 0) {
            t.appendText("\n");
            t.appendText(addstr(i));
            t.appendText("   ");
          }
          b = buf[i++];
          if (b != 0) t.appendText(""+(char)b);
        }
      } break;
      case 8: { // pointer table
        int n = getInt(i);
        i += 2;
        for (int j=0; j<n; j++) {
          t.appendText("\n");
          int a = getInt(i);
          t.appendText(addstr(a));
          t.appendText("->");
          a = getInt(a);
          t.appendText(addstr(a));
          t.appendText(" ");
          i += 2;
          while (buf[a] != 0) t.appendText(""+(char)buf[a++]);
        }
      } break;
      case 1: // object
      case 6: // something like object
      {
        t.appendText("\n");
        int a = getInt(start+0x12);
        while (buf[a] != 0) t.appendText(""+(char)buf[a++]);
      } // fall through - not enough info to do otherwise
      default: {
        int format = 0;
        while (i < end) {
          if ((format & 0xf) == 0) {
            t.appendText("\n");
            t.appendText(addstr(format));
            t.appendText(" ");
            t.appendText(addstr(i));
            t.appendText("   ");
          } else if ((format & 0xf) == 8)
            t.appendText("   ");
          else t.appendText(" ");
          t.appendText(bytestr(buf[i]));
          i++;
          format++;
        }
      }
    }
  }
}

public class script {
  byte buf[];
  Vector blocks = new Vector();

  public int size() {
    return blocks.size();
  }

  ScriptBlock block(int i) {
    return (ScriptBlock)blocks.elementAt(i);
  }

  public short getInt(int i) {
    short lo = (short)(0xff & (short)buf[i]);
    short hi = (short)buf[i+1];
    return (short)((hi<<8) | lo);
  }
  
  public script(byte buf[]) {
    this.buf = buf;
    int i=0;
    short type;
    while ((type = getInt(i)) != 0) {
      ScriptBlock block = new ScriptBlock(buf, i);
      i += block.len();
      blocks.addElement(block);
    }
  }

  public static void main(String args[]) {
    if (args.length != 1) {
      System.out.println("Please provide exactly one filename.");
      System.exit(0);
    }
    try {
      byte buf[] = new byte[(int)new File(args[0]).length() - 2];
      DataInputStream in = new DataInputStream(new FileInputStream(args[0]));
      in.skipBytes(2);
      in.readFully(buf);
      script s = new script(buf);
      for (int i = 0; i<s.size(); i++) {
        Frame f = new Frame();
        f.setLayout(new BorderLayout());
        f.setTitle(args[0]+" - block "+i);
        TextArea t = new TextArea();
        t.setFont(new Font("Courier", Font.PLAIN, 12));
        t.setBackground(new Color(208, 136, 56));
        s.block(i).display(t);
        f.add("Center", t);
        f.resize(470, 300);
        f.show();
      }
    } catch (IOException e) {
      e.printStackTrace();
    }
  }
}

