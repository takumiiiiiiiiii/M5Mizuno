//g++ -O3 main.cpp -std=c++11 -framework OpenGL -framework GLUT -Wno-deprecated
//g++ main.cpp -framework OpenGL -framework GLUT -I/opt/homebrew/include -L/opt/homebrew/lib -lalut -framework OpenAL -Wno-deprecated
#include <iostream>
#include <GLUT/glut.h>  //OpenGL
#include <math.h>  //数学関数
#include <unistd.h>  //シリアル通信用
#include <fcntl.h>  //シリアル通信用
#include <termios.h>  //シリアル通信用
#include <AL/alut.h>//OpenAL

//シリアルポート用
#define DEV_NAME "/dev/cu.M5stick_No3"  //M5StickC Plus2接続シリアルポート
#define BAUD_RATE B115200  //通信速度
#define BUFF_SIZE 4096

//関数名の宣言
void initGL();
void initAL();
void display();
void reshape(int w, int h);
void keyboard(unsigned char key, int x, int y);
void timer(int value);
void initSerial();
int getSerialData();
void analyzeBuffer();

//グローバル変数

int winW, winH;  //ウィンドウサイズ
double fr = 60.0;  //フレームレート
double floorW= 1076.0,floorH = 691.0;//床の投影面サイズ
double deg = 0.0;//物体の回転角度
ALuint soundSource;//音源


//シリアル通信関係
int fd;  //シリアルポート
char bufferAll[BUFF_SIZE];  //蓄積バッファデータ
int bufferPoint = 0;  //蓄積バッファデータサイズ
double val[6];  //受信データ

//メイン関数
int main(int argc, char *argv[])
{
    initSerial();  //シリアルポート初期化
    glutInit(&argc, argv);  //OpenGL/GLUTの初期化
    alutInit(&argc, argv);  //OpenGL/GLUTの初期化
    initGL();  //初期設定
    initAL();  //OpenAL初期設定
    glutMainLoop();  //イベント待ち無限ループ
    
    return 0;
}

//ディスプレイコールバック関数
void display()
{
    glClear(GL_COLOR_BUFFER_BIT);  //画面消去
    
    //1つの四角形の描画
    glColor3d(1.0, fabs(val[5])/100.0, 0.0);  //色（R,G,B，0~1）
    glPushMatrix();
    glTranslated(0.0, 0.0, 0.0);  //中心座標(px,py,0)
    glRotated(deg,0.0,0.0,1.0);//z軸周りにdeg度回転
    glScaled(100.0, 100, 1.0);  //サイズ(sx,sy,1)・・・val[0]の値でy方向のサイズが変化
    glutSolidCube(1.0);  //四角形
    glPopMatrix();
    
    glFlush();  //描画実行（display()関数の一番最後に1回だけ記述）
}

//タイマーコールバック関数
void timer(int value)
{
    glutTimerFunc(1000/fr, timer, 0);  //タイマー再設定・・・timer()関数が1000/fr秒ごとに呼び出される
    glutPostRedisplay();  //ディスプレイイベント強制発生・・・display()関数が呼び出される
    
    getSerialData();  //加速度：val[0],val[1],val[2]，角速度val[3], val[4], val[5]を入手
    printf("(%f, %f, %f) (%f, %f, %f)\n", val[0], val[1], val[2], val[3], val[4], val[5]);
    if(fabs(val[0])>3){
        
        alSourcePlay(soundSource);
    }
    
    alSourcef(soundSource,AL_PITCH,fabs(val[0]+1.0));//ピッチ設定
    deg +=val[5]/fr;//物体の回転角度の更新　フレームレートで割る
}

//シリアルポートからデータ取得
int getSerialData()
{
    //シリアルポートからの入力用
    char buffer[BUFF_SIZE];
    //buffer[]に受信データ格納
    int len = read(fd, buffer, BUFF_SIZE);
    
    //bufferAll[]に蓄積
    for (int i=0; i<len; i++) {
        bufferAll[bufferPoint] = buffer[i];  //bufferAll[]にbuffer[]を蓄積
        bufferPoint++;  //バッファサイズ追加
        
        if (buffer[i]=='\n') {  //読み込んだ文字が'\n'(改行)なら受信データの最後
            analyzeBuffer();  //データの解析
            bufferPoint = 0;  //bufferAll[]をリセット
        }
    }
    
    return len;
}

//bufferAll[]分析
void analyzeBuffer()
{
    char inData[6][BUFF_SIZE];
    int dataID = 0;  //データ番号
    int j = 0;  //文字位置
    
    for (int i=0; i<bufferPoint; i++) {
        inData[dataID][j] = bufferAll[i];  //bufferALl[]の値をinData[][j]に1文字ずつ格納
        
        if (inData[dataID][j]==',') {  //カンマ（数値データの区切り）のとき
            inData[dataID][j] = '\0';  //文字列inData[dataID]完成
            dataID++;  //次の数値データ（文字列）に移動
            if (dataID==6) break;  //6を超えないように
            j = 0;  //次の文字に移動
        }
        else if (inData[dataID][j]=='\n') {  //改行コード（受信データセットの最後）のとき
            inData[dataID][j] = '\0';  //文字列生成
            dataID++;  //次の数値データに移動
            break;  //全データの取得完了
        }
        else {  //データの途中
            j++;  //次の文字に移動
        }
    }
    
    if (dataID==6) {  //データが完全に揃っているとき
        //inData[i][]（i=0~5）の文字列を数値化してval[i]に格納
        for (int i=0; i<6; i++) {
            //文字列を小数に変換
            val[i] = atof(inData[i]);
        }
    }
}

//初期化関数
void initGL()
{
   //描画ウィンドウ生成
    glutInitDisplayMode(GLUT_RGBA);  //ディスプレイモードの指定
    glutInitWindowSize(floorW, floorH);  //ウィンドウサイズの指定
    glutCreateWindow("CG");  //ウィンドウの生成
    
    //コールバック関数の指定
    glutDisplayFunc(display);  //ディスプレイコールバック関数の指定（"display()"）
    glutReshapeFunc(reshape);  //リサイズコールバック関数の指定（"reshape()"）
    glutKeyboardFunc(keyboard);  //キーボードコールバック関数の指定（"keyboard()"）
    glutTimerFunc(1000/fr, timer, 0);  //タイマーコールバック関数（"timer"）
    
    //各種設定
    glClearColor(0.0, 0.0, 0.2, 1.0);  //ウィンドウクリア色の指定（RGBA）
}
//OpenAL初期設定
void initAL()
{
    ALuint soundBuffer; //バッファ
    alGenBuffers(1,&soundBuffer);//バッファ生成

    //音源生成
    soundBuffer = alutCreateBufferFromFile("pon.wav");//バッファにサウンドファイル取り込み
    alGenSources(1,&soundSource);//音源生成
    alSourcei(soundSource,AL_BUFFER,soundBuffer);//音源にバッファを結びつけ
    alSourcei(soundSource,AL_LOOPING,AL_TRUE);//ループ再生設定(オフ)
    alSourcei(soundSource,AL_PITCH,1.0);//ピッチ設定
    alSourcei(soundSource,AL_GAIN,1.0);//音量設定（1が最大)
}
//リシェイプコールバック関数
void reshape(int w, int h)
{
    glViewport(0, 0, w, h);  //ウィンドウ内の描画領域(ビューポート)の指定
    //投影変換の設定
    glMatrixMode(GL_PROJECTION);  //変換行列の指定（設定対象は投影変換行列）
    glLoadIdentity();  //行列初期化
    gluOrtho2D(-floorW/2, floorW/2, 0.0, floorH);  //二次元座標設定（平行投影）
}

//キーボードコールバック関数(key:キーの種類，x,y:座標)
void keyboard(unsigned char key, int x, int y)
{
    switch (key) {
        case 27:
            exit(0);  //終了
        case 's':
            alSourcePlay(soundSource);
            break;
        default:
            break;
    }
}

//シリアルポート初期化
void initSerial()
{
    struct termios tio;
    
    //シリアルポートオープン
    fd = open(DEV_NAME, O_RDWR | O_NONBLOCK );
    if(fd<0) { //オープン失敗
        printf("ERROR on device open\n");
        exit(1);
    }
    printf("init serial port\n");
    
    //初期設定
    memset(&tio, 0, sizeof(tio));
    tio.c_cflag = CS8 | CLOCAL | CREAD;
    tio.c_cc[VTIME] = 100;
    //ボーレート設定
    cfsetispeed(&tio, BAUD_RATE);
    cfsetospeed(&tio, BAUD_RATE);
    //デバイス設定実施
    tcsetattr(fd, TCSANOW, &tio);
}
