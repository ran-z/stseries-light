#include <stdio.h>
#include <hidapi/hidapi.h>
#include <QtCore>

QTextStream * out;

// transfer HID devices
void enumerate_hid()
{
    hid_device_info * hinfo;
    hid_device_info * enumerate;

    enumerate = hid_enumerate(0,0);
    if(enumerate != NULL)
    {
        hinfo = enumerate;

        while(hinfo != NULL)
        {
            * out << QCoreApplication::tr("Device: %1:%2 - %3\n").arg(QString::number(hinfo->vendor_id, 16),
                        QString::number(hinfo->product_id,16), hinfo->path);
            * out << QCoreApplication::tr("    Manufacturer: %1\n").arg(QString::fromWCharArray(hinfo->manufacturer_string));
            * out << QCoreApplication::tr("    Device:    %1\n").arg(QString::fromWCharArray(hinfo->product_string));

            hinfo = hinfo->next;
        }

        hid_free_enumeration(enumerate);
    }
    else
        * out << QCoreApplication::tr("The device list is empty\n");
}

// necessary transfers
class kl_const
{
  public:
    enum regions      // keyboard area
    {
      left   = 1,
      middle = 2,
      right  = 3
    };

    enum colors       // цвета
    {
      off = 0,
      red = 1,
      orange = 2,
      yellow = 3,
      green = 4,
      sky = 5,
      blue = 6,
      purple = 7,
      white = 8
    };

    enum levels     // levels of illumination
    {
      light = 3,
      low = 2,
      med = 1,
      high = 0
    };

    enum modes      // backlight modes
    {
      normal = 1,
      gaming = 2,
      breathe = 3,
      demo = 4,
      wave = 5
    };
};

// set the mode
void set_mode(hid_device * dev, kl_const::modes mode)
{
    unsigned char commit[8];

    // check the input parameter
    if (mode != kl_const::normal && mode != kl_const::gaming &&
       mode != kl_const::breathe && mode != kl_const::demo && mode != kl_const::wave)
       mode = kl_const::normal;

    commit[0] = 1;
    commit[1] = 2;
    commit[2] = 65; // commit
    commit[3] = (unsigned char) mode; // set hardware mode
    commit[4] = 0;
    commit[5] = 0;
    commit[6] = 0;
    commit[7] = 236; // EOR

    hid_send_feature_report(dev, commit, 8);
}

// set the color
void set_color(hid_device * dev, kl_const::regions region, kl_const::colors color, kl_const::levels level)
{
    unsigned char activate[8];

    // check the region
    if(region != kl_const::left && region != kl_const::middle && region != kl_const::right)
        return;

    // check the color
    if(color != kl_const::off && color != kl_const::red && color != kl_const::orange &&
       color != kl_const::yellow && color != kl_const::green && color != kl_const::sky &&
       color != kl_const::blue && color != kl_const::purple && color != kl_const::white)
        return;

    // check the level
    if(level != kl_const::light && level != kl_const::low &&
       level != kl_const::med && level != kl_const::high)
        return;

    activate[0] = 1;
    activate[1] = 2;
    activate[2] = 66; // set
    activate[3] = (unsigned char) region;
    activate[4] = (unsigned char) color;
    activate[5] = (unsigned char) level;
    activate[6] = 0;
    activate[7] = 236; // EOR (end of request)

    hid_send_feature_report(dev, activate, 8);
}

int main(int argc, char *argv[])
{
    int res;
    hid_device * hiddev;
    bool arg_error = false;
    QString AllowedParms = "-l,-off,-red,-orange,-sky,-blue,-yellow,-green,-purple,-white,";
    QString Arg1;

    // initialize
    QCoreApplication a(argc, argv);
    QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
    QTextStream _out(stdout, QIODevice::WriteOnly);
    _out.setCodec(QTextCodec::codecForLocale());
    out = & _out;

    // matrix matching options
    QMap <QString, kl_const::regions> MapRegions;
    QMap <QString, kl_const::colors>  MapColors;
    QMap <QString, kl_const::levels>  MapLevels;

    MapRegions.insert("left",   kl_const::left);
    MapRegions.insert("middle", kl_const::middle);
    MapRegions.insert("right",  kl_const::right);

    MapColors.insert("off",    kl_const::off);
    MapColors.insert("red",    kl_const::red);
    MapColors.insert("orange", kl_const::orange);
    MapColors.insert("sky",    kl_const::sky);
    MapColors.insert("blue",   kl_const::blue);
    MapColors.insert("yellow", kl_const::yellow);
    MapColors.insert("green",  kl_const::green);
    MapColors.insert("purple", kl_const::purple);
    MapColors.insert("white",  kl_const::white);

    MapLevels.insert("light",  kl_const::light);
    MapLevels.insert("low",    kl_const::low);
    MapLevels.insert("med",    kl_const::med);
    MapLevels.insert("high",   kl_const::high);

    // check parameters
    if(argc < 2 || argc > 3)
        arg_error = true;

    if(argc == 2)
    {
      Arg1 = argv[1];

      if(Arg1 == "-p")
        arg_error = true;

      if(!(AllowedParms.contains(Arg1 + ",")))
        arg_error = true;
    }

    if(argc == 3)
    {
        Arg1 = argv[1];
        if(Arg1 != "-p")
          arg_error = true;
    }

    if(arg_error)
    {
      *out << a.tr("Usage: \n");
      *out << a.tr(" -l       List HID devices (we need 0x1770, 0xff00)\n");
      *out << a.tr(" -off     Switch off all\n");
      *out << a.tr(" -red     Red light\n");
      *out << a.tr(" -orange  Orange light\n");
      *out << a.tr(" -yellow  Yellow light\n");
      *out << a.tr(" -green   Green light\n");
      *out << a.tr(" -sky     Sky light\n");
      *out << a.tr(" -blue    Blue light\n");
      *out << a.tr(" -purple  Purple light\n");
      *out << a.tr(" -white   White light\n");
      *out << a.tr(" -p preset Preset from the configuration file\n\n");
      return -1;
    }

    res = hid_init();
    if(res != 0)
    {
        *out << a.tr("Failed to initialize HIDAPI: %1").arg(res);
        return -1;
    }

    if(Arg1 == "-l")
      enumerate_hid();
    else
    {
        // obtain the desired device // 0x1770, 0xff00 make two attempts
        hiddev = hid_open(0x1770, 0xff00, 0);
        if(hiddev == NULL)
        {
            hid_exit();
            res = hid_init();
            if(res != 0)
            {
                *out << a.tr("Failed to initialize HIDAPI: %1").arg(res);
                return -1;
            }

            hiddev = hid_open(0x1770, 0xff00, 0);
            if(hiddev == NULL)
            {
                *out << a.tr("Error opening the device!\n");
                hid_exit();
                return -1;
            }
        }

        if(argc == 3 && Arg1 == "-p")
        {
          // read the data form the configuration file
          QFile f("/etc/w7/key-light.conf");
          if(f.open(QIODevice::ReadOnly))
          {
            QString Line, preset(argv[2]);
            QStringList fields;
            kl_const::regions region = kl_const::left;
            kl_const::colors color   = kl_const::white;
            kl_const::levels level   = kl_const::high;
            bool preset_found = false;
            QTextStream in(&f);
            int LineCount = 0;

            in.setCodec("UTF-8");
            while (!in.atEnd())
            {
               LineCount++;
               Line = in.readLine().trimmed();
               if(Line.isEmpty() || Line.left(1) == "#")
                   continue;

               fields = Line.split(QRegExp("[ \t]{1,100}"));
               if(fields.size() != 4)
                 continue;

               if(fields.at(0) == preset)
               {
                 preset_found = true;

                 // region
                 if(MapRegions.contains(fields.at(1)))
                    region = MapRegions.value(fields.at(1));
                 else
                 {
                   *out << a.tr("Bad region value: %1").arg(LineCount);
                   arg_error = true;
                 }

                 // color
                 if(MapColors.contains(fields.at(2)))
                    color = MapColors.value(fields.at(2));
                 else
                 {
                   *out << a.tr("Bad color value: %1").arg(LineCount);
                   arg_error = true;
                 }

                 // intensity
                 if(MapLevels.contains(fields.at(3)))
                    level = MapLevels.value(fields.at(3));
                 else
                 {
                   *out << a.tr("Bad level value: %1").arg(LineCount);
                   arg_error = true;
                 }

                 if(!arg_error)
                   set_color(hiddev, region, color, level);
               }
            }

           set_mode(hiddev,  kl_const::normal);
           f.close();

           if(!preset_found)
           {
               *out << a.tr("Preset setting wasn't found\n");
               arg_error = true;
           }
          }
          else
          {
              *out << a.tr("Error opening configuration file /etc/w7/key-light.conf\n");
              arg_error = true;
          }
        }
        else
        {
           // color is specified in the parameter
           kl_const::colors color;

           Arg1.remove(0,1);

           if(MapColors.contains(Arg1))
             color = MapColors.value(Arg1);
           else
             color = kl_const::white;

           set_color(hiddev, kl_const::left,   color, kl_const::high);
           set_color(hiddev, kl_const::middle, color, kl_const::high);
           set_color(hiddev, kl_const::right,  color, kl_const::high);
           set_mode(hiddev,  kl_const::normal);
        }

        hid_close(hiddev);
    }

    hid_exit();

    if(arg_error)
      return -1;
    else
      return 0;
}
