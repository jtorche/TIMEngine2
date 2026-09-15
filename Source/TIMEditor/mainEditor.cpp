#include <QApplication>
#include <QTextStream>
#include "TIMEditor/EditorWindow.h"
#include "TIMEditor/RendererThread.h"
#include "TIMEditor/VRRoomTools.h"
#undef main

// TIMEditor --vrRooms <sceneFolder> [--noWrite] [--roomSize N] : VR room generation without the GUI,
// same as the menu VR Room > Generate VR rooms with the default parameters. Run it from the Data folder.
static int generateVRRooms(int argc, char* argv[])
{
    VRRoomGraph::Parameters graphParam;
    VRRoomWriter::Parameters writerParam;
    graphParam.sceneFolder = argv[2];

    bool write = true;
    for(int i=3 ; i<argc ; ++i)
    {
        if(QString(argv[i]) == "--noWrite")
            write = false;
        else if(QString(argv[i]) == "--roomSize" && i+1 < argc)
            graphParam.roomSize = writerParam.roomSize = QString(argv[++i]).toFloat();
    }

    QString log;
    VRRoomGraph graph;
    bool ok = graph.build(graphParam, log);

    QStringList written;
    if(ok && write)
        ok = VRRoomWriter::writeRooms(graph, writerParam, written, log);

    QTextStream(stdout) << log;
    return ok ? 0 : 1;
}

int main(int argc, char *argv[]) {
    if(argc >= 3 && QString(argv[1]) == "--vrRooms")
        return generateVRRooms(argc, argv);

    QApplication a(argc, argv);

    EditorWindow w;
    w.show();

    return a.exec();
}
