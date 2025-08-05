#define _POSIX_C_SOURCE 200809L
// ⬆️ POSIX.1-2008 표준 함수들을 모두 사용할 수 있도록 설정
/*
 * 졸업작품 Visual Shell - 11월 14일까지의 진행사항에서 부터의 개선 시작!
 */

/*------------------------------
 * ***** 웹사이트 *****
 * 1) NCURSES Programming HOWTO
 *    http://wiki.kldp.org/wiki.php/NCURSES-Programming-HOWTO
 *    위 웹사이트의 Example 22번을 기반으로 여러가지를 바꿨습니다.
 *
 * 2) Putty 에서 외곽선 문제 해결.
 *    http://forums.freebsd.org/showthread.php?t=15688
 *    NCURSES_NO_UTF8_ACS 라는 이름 값은 1인  환경변수가 설정되어있어야
 *    UTF-8 모드의 Putty에서 border혹은 box로 표시되는 외곽선이 lqqqk 방식
 *    으로 표시되지 않기 때문에... 소스코드안에 환경변수를 설정해주는
 *    코드를 넣었습니다.
 *
 * 3) scandir()사용에서 필터 사용
 *
http://cboard.cprogramming.com/c-programming/91925-argument-3-confusion-scandir.html
 *    사용에 의미없는 "."또는 ".."을 제거하기위해서.
 *
 *
 * ***** 교제 목록 *****
 *
 * 1) UNIX 시스템 프로그래밍 Second Edition
 *    Keith Haviland, Dina Gray, Ben Salama 원저
 *    조유근 역
 *
 * 2) UNIX SYSTEMS Programming 통신 병행성 그리고 쓰레드
 *    Kay A. Robbins, Steven Robbins 공저
 *    주민규, 권상호, 윤종수 공역
 *
 * 3) UNIX 고급 프로그래밍 2판
 *    [Advanced Programming in the UNIX Environment Second Edition]
 *    리처드 스티븐스, 스티븐 레이고 지음
 *    류광 옮김
 *
 * 4) Programmer's Guide to nCurses
 *    Dan Gookin
 *
 * 5) 유닉스, 리눅스 프로그래밍 필수 유틸리티
 *    :vi, make, gcc, gdb, cvs, rpm
 *    백창우 저
 *    gdb나 vi에서 뭔가 막힐때 읽었는데 많은 도움이 되었습니다.
--------------------------------*/

#include <menu.h>
#include <ncurses.h>
#include <panel.h>
#include <pwd.h>  // 3)교제p196
#include <stdio.h>
#include <stdlib.h>
// struct passwd *getpwuid(uid_t uid);

#include <grp.h>  // 3)교제p200
// struct group *getgrgid(gid_t gid);

#include <dirent.h>     // DIR *opendir(const char *name);  1)교제 94p
#include <locale.h>     // 한글 입출력을 위해 setlocale()을 써야함으로 필요.
#include <string.h>     // strcat(), strlen()
#include <sys/types.h>  //
#include <time.h>       // 개체의 수정한시간을 나타내게 하기 위해.
#include <unistd.h>     // long pathconf(const char *path, int name);
// struct dirent *readdir (DIR *dirptr);   1)교제 95p
// void rewinddir(DIR *dirptr);
//
#include <errno.h>
#include <signal.h>  // int sigaction( int sig, const struct sigaction *restrict act
#include <sys/stat.h>
#include <sys/wait.h>  // pid_t wait(int *status);
//        ,struct sigaction *restrict oact);

#include <setjmp.h>  // void siglongjmp(sigjmp_buf env, int val);     3)교제 391p
// int sigsetjmp(sigmjmp_buf env, int savesigs); 3)교제 391p
//
#include <fcntl.h>  // STDOUT_FILENO 및 open()에서 사용할 상수들.

#define BUFSIZE 1024  // 복사할 때의 버퍼 크기

typedef struct clip_entity {
  char *abspath;  // 절대경로
  char *name;     // 개체 이름
} CLIP_ENTITY;

// 전역변수 선언//
static sigjmp_buf position;
static volatile sig_atomic_t mark_val;  // static 이 파일 내에서만 한정
// volatile 키워드   http://skyul.tistory.com/337  컴파일러가 최적화 하지 않음.

chtype volatile main_ipt_ch;  // getch 반환값 받음 /  2바이트 이상의 유니코드
                              // 문자도 받을 수 있음.
// Ctrl + [알파벳]입력을 받기 위해 int에서 chtype로 변경

// 전역변수 끝 //

void print_in_middle(WINDOW *win, int starty, int startx, int width,
                     char *string, chtype color);

int print_current_entity(const char *entity_name, const int pce_mode,
                         WINDOW *entity_detail_display_win);

void entity_size_calc(const char *entity_name, char *rtn_entity_size_chr);

void getCurrDir(char **absDIRstr, long *maxpath);

void execFunc(const char *filename);  // Exec()류 함수들을 통해 실행파일
                                      // 실행하기 위한 목적의 함수.

void execFilenameRev(
    const char *filename, const int name_num,
    char **exec_filename);  // 실행파일명을 "./실행파일명" 으로 만들기.
// exec_filename에 동적 메모리 할당됨 주의

void ShortCutWinDisplay(
    WINDOW *shortcut_display_win);  // 단축키를 표시할 윈도우

void EntityListWinDisplay(WINDOW *entity_list_win);

void DirectoryWinDisplay(WINDOW *directory_display_win, const char *absDIRstr);

void SelectedItemClipBoard(
    int selectItemCount[],    /* 선택된 아이템의 갯수 */
    char *absDIRstr,          /* 단축키가 눌려진 시점의 절대경로 */
    ITEM ***my_items,         /* 아이템의 배열의 주소  */
    MENU **my_menu,           /* 메뉴 포인터의 주소.  */
    CLIP_ENTITY **clip_items, /* 아이템의 절대경로와 이름을 저장할 포인터 */
    char **bakAbsDIRstr       /* 단축키가 눌려진 시점의 절대경로 백업  */
);

void PasteItemClipBoard(
    int selectItemCount[],   /* 선택된 아이템의 갯수 배열  0번은 이전 1번은 현재
                              */
    char *absDIRstr,         /* Ctrl+V단축키가 눌려진 시점의 절대경로 */
    CLIP_ENTITY **clip_items /* 아이템의 절대경로와 이름을 저장할 포인터 */
);

void DeleteItemClipBoard(
    int selectItemCount[],   /* 선택된 아이템의 갯수 배열  0번은 이전 1번은 현재
                              */
    CLIP_ENTITY **clip_items /* 아이템의 절대경로와 이름을 저장할 포인터 */
);

void Dlg_Chmod(WINDOW *dlg_win_chmod, int selectItemCount[],
               CLIP_ENTITY **clip_items);
// 권한변경 다이얼로그 박스 윈도우 표시 // 다중 개체 변경 가능

void Dlg_Mkdir(WINDOW *dlg_win_mkdir, const char *curDirName, char *usrIptName);
// 디렉토리 생성 다이얼로그 박스 윈도우 표시

void Dlg_Rename(WINDOW *dlg_win_rename, const char *prevName, char *newName);
// 이름 변경 다이얼로그 박스 윈도우 표시.

void initMenu(int *n_choices, struct dirent ***d_ptrArray, ITEM ***my_items,
              char ***rtn_entity_size_chr, WINDOW **entity_list_win,
              MENU **my_menu, const char *absDIRstr) {
  int dot_no_select();   // scandir의 필터 등록 함수. "."
  int ddot_no_select();  // "." ".." 둘다 제거.

  int (*filter_func)(
      const struct dirent *); /* scandir filter함수포인터를 저장할 변수.*/

  int i, par_x, par_y, max_x, max_y;
  int pce_rtn_val = 0;  // pce_rtn값을 임시 저장할 변수

  // Initialize items

  if (strcmp(absDIRstr, "/") == 0) {  // 서브디렉토리에서는  "." 만
    filter_func =
        ddot_no_select;  // 루트디렉토리에서는 "." ".."을 제거하고 싶어서.
  } else {
    filter_func = dot_no_select;
  }

  if ((*n_choices = scandir(".", d_ptrArray, filter_func, alphasort)) < 0) {
    endwin();
    perror("scandir() 오류");
    exit(-1);
  }

  if ((*my_items = (ITEM **)calloc(*n_choices + 1, sizeof(ITEM *))) == NULL) {
    endwin();
    perror("calloc()오류 - 위치 initMenu() ");
    exit(-2);
  }

  if ((*rtn_entity_size_chr = (char **)calloc(*n_choices, sizeof(char *))) ==
      NULL) {
    endwin();
    perror("calloc()오류 - 위치 initMenu() ");
    exit(-3);
  }

  for (i = 0; i < *n_choices; i++) {
    if (((*rtn_entity_size_chr)[i] = (char *)malloc(sizeof(char) * (16))) ==
        NULL) {
      endwin();
      perror("malloc()오류 - 위치 initMenu() - for");
      exit(-4);
    }
  }  // 엄청나게 큰 기가바이트 단위가 아닌이상
  // 문제는 없기 때문에, 그냥 고정값 16넣음.

  for (i = 0; i < *n_choices; i++) {
    pce_rtn_val = print_current_entity((((*d_ptrArray)[i])->d_name), 1, NULL);

    switch (pce_rtn_val) {
      case 0:
        entity_size_calc(((*d_ptrArray)[i])->d_name, (*rtn_entity_size_chr)[i]);
        // BYTE 사이즈를 KB, MB, GB 단위로 축약해주는 함수.
        (*my_items)[i] =
            new_item(((*d_ptrArray)[i])->d_name, (*rtn_entity_size_chr)[i]);
        // [개체명] [크기]를 메뉴명으로 삽입
        break;

      case 1:
        (*my_items)[i] = new_item(((*d_ptrArray)[i])->d_name, "[디렉토리]");
        break;

      case 2:
        entity_size_calc(((*d_ptrArray)[i])->d_name, (*rtn_entity_size_chr)[i]);
        (*my_items)[i] =
            new_item(((*d_ptrArray)[i])->d_name, (*rtn_entity_size_chr)[i]);

        // 실행 속성에 대해 메뉴 사용자 포인터 저장 부분---

        set_item_userptr((*my_items)[i], execFunc);  // 웹1) Example 24 참고.

        //-------------------------------------------------
        break;

    }  // switch end
  }  // for end

  getmaxyx(*entity_list_win, max_y, max_x);
  getparyx(*entity_list_win, par_y, par_x);

  (*my_items)[*n_choices] =
      (ITEM *)NULL;  // 결국은 my_items[]의 마지막에 NULL을 넣는다.
  *my_menu = new_menu((ITEM **)(*my_items));

  set_menu_win((*my_menu), *entity_list_win);
  set_menu_sub((*my_menu), derwin(*entity_list_win, max_y - 4, max_x - 4,
                                  par_y + 4, par_x + 3));

  // 파일 선택 표시를 #으로 하기 위해서.
  set_menu_mark(*my_menu, "# ");

  if (max_y - 5 < *n_choices) {
    // 메뉴아이템의 의 열과 행 갯수
    set_menu_format(*my_menu, max_y - 4, 2);
  } else {
    set_menu_format(*my_menu, max_y - 4, 1);
  }

  // 메뉴아이템과 설명의 간격 3, 아이템간 열간격 5  공백넣기
  set_menu_spacing(*my_menu, 3, 0, 5);

  // Make the menu multi valued
  // 다중 선택이 가능해야하므로.
  menu_opts_off(*my_menu, O_ONEVALUE);

  // 메뉴가 열로서 정렬되도록
  menu_opts_off(*my_menu, O_ROWMAJOR);

  if (strcmp(absDIRstr, "/") !=
      0)  // 루트디렉터리가 아닐때 ".."이 선택되지 않도록 함.
  {
    item_opts_off((*my_items)[0], O_SELECTABLE);
  }

  post_menu(*my_menu);

}  // initMenu() End

void freeDynamicMem(MENU **my_menu, ITEM ***my_items,
                    struct dirent ***d_ptrArray, char ***rtn_entity_size_chr,
                    char **absDIRstr, const int *n_choices) {
  int i;

  unpost_menu(*my_menu);
  free_menu(*my_menu);
  *my_menu = (MENU *)NULL;

  for (i = 0; i < *n_choices; i++) {
    if ((free_item((*my_items)[i])) != E_OK) {
      endwin();
      perror("free_item()오류 - 위치: freeDynamicMem() ");
      exit(-100);
    }

    (*my_items)[i] = (ITEM *)NULL;
  }
  (*my_items) = (ITEM **)NULL;

  for (i = 0; i < *n_choices; i++) {
    free((*d_ptrArray)[i]);
    (*d_ptrArray)[i] = (struct dirent *)NULL;
  }
  free(*d_ptrArray);  // scandir()을 사용했을 때 동적 메모리 해제.
  (*d_ptrArray) = (struct dirent **)NULL;

  for (i = 0; i < *n_choices; i++) {
    free((*rtn_entity_size_chr)[i]);
    (*rtn_entity_size_chr)[i] = (char *)NULL;
  }
  free(*rtn_entity_size_chr);
  *rtn_entity_size_chr = (char **)NULL;

  if (absDIRstr !=
      (char **)NULL)   // 메모리 할당해제가 필요없을 때는 함수에 NULL로 전달하고
  {                    // 처리부에서 if문 붙임.
    free(*absDIRstr);  // 시작 경로 문자열이 들어가있음.
    *absDIRstr = (char *)NULL;
  }

}  // 동적 메모리를 사용하는 포인터 변수가 잡고있는 메모리들을 자유롭게~~~

int main(void) {
  setlocale(LC_ALL, "C.UTF-8");           // 안전한 UTF-8 로케일 설정
  setenv("NCURSES_NO_UTF8_ACS", "1", 0);  // 환경변수를 설정해야하는 이유 - 웹2)

  PANEL *my_panels[4];  // 0:entity_list, 1:mkdir_dlg, 2:rename_dlg, 3:chmod_dlg
  PANEL *top;

  WINDOW *entity_list_win,         // 현재 디렉토리 안의 개체를 출력하는
                                   // 메뉴리스트를 붙일 윈도우
                                   //
      *shortcut_display_win,       //
      *entity_detail_display_win,  //
      *directory_display_win,      //
      *dlg_win_mkdir,              //
      *dlg_win_rename,             //
      *dlg_win_chmod;

  ITEM **my_items;  // ITEM에 대한 포인터 배열의 첫번째 주소를 저장할
                    // 2중 포인터 변수.
  MENU *my_menu;
  int n_choices;  // 메뉴 아이템의 갯수를 저장할 변수 /
  // int min_x, min_y, max_x, max_y;   // getbegyx(), getmaxyx()와 같이 사용할
  // 함수

  char *exec_filename;  // "./"이 붙은 실행 파일명 저장 공간.

  void ctrlC_SIGINT(int signo);  // CTRL+C가입력 되었을 때 취해질 행동

  static struct sigaction act, oldact;  //

  struct dirent **d_ptrArray;

  char **rtn_entity_size_chr;  // 개체의 변환된 크기를 저장할 문자열
  // 메뉴를 출력할 때 입력한 메뉴아이템 값을
  // 다른 곳에 옮기는 것이아니고,
  // 주소만을 저장하기 때문에,
  // 이 부분을 동적메모리에 따로 할당할 필요가 있음.

  long maxpath;     // 최대 경로 길이를 저장할 변수
  char *absDIRstr;  // 경로 문자열을 저장할 char * 인데
                    //  동적 메모리를 사용하여 할당함.

  CLIP_ENTITY *clip_items;
  int selectItemCount[2];
  char *bakAbsDIRstr;
  int copy1move2 = 0;

  shortcut_display_win = (WINDOW *)NULL;
  entity_detail_display_win = (WINDOW *)NULL;
  entity_list_win = (WINDOW *)NULL;
  dlg_win_mkdir = (WINDOW *)NULL;

  d_ptrArray = (struct dirent **)NULL;
  my_items = (ITEM **)NULL;
  rtn_entity_size_chr = (char **)NULL;
  my_menu = (MENU *)NULL;
  absDIRstr = (char *)NULL;
  clip_items = (CLIP_ENTITY *)NULL;
  bakAbsDIRstr = (char *)NULL;

  char usrIptName[100];  // 사용자가 입력한 파일명 또는
                         // 디렉토리 이름을 받을 공간.

  selectItemCount[0] = 0;  // 과거 선택
  selectItemCount[1] = 0;  // 현재 선택

  // SIGINT를 수신했을 때 취해질 행동 지정
  act.sa_handler = ctrlC_SIGINT;

  // 완전히 찬 마스크를 하나 생성한다.
  sigfillset(&(act.sa_mask));

  if (sigaction(SIGINT, &act, NULL) == -1) {
    endwin();
    perror("위치: main(), sigaction()오류");
    exit(-5);
  }
  // sigaction 호출전에는, SIGINT가 프로세스를 종료시킬 수 있다.(초기값 행동)

  // Initialize curses
  initscr();  // Curses 초기화

  // UTF-8 및 wide character 지원 설정
  if (has_colors() == FALSE) {
    endwin();
    printf("터미널에서 컬러를 지원하지 않습니다.\n");
    exit(1);
  }

  start_color();  // 컬러모드 사용
  cbreak();       // 라인 버퍼링 해제
  noecho();       // 반향 제거

  // UTF-8 입력 모드 설정 (추가 보장)
  set_escdelay(25);  // ESC 지연시간을 짧게 설정
  // raw();    // SIGQUIT, SIGINT, SIGSUSP, SIGTSTP, 등의 입력을 무시
  //  해당 키입력시 그대로 ^C, ^\등으로 받을 수 있음.
  //  그러나 nCurses모드 안에서만 유효. raw() 교제4) 413p
  keypad(stdscr, TRUE);  // 기본화면 stdscr에 기능키 사용가능
  // curs_set(0);  // 커서를 보이지 않게. 교제4) 238p

  init_pair(1, COLOR_YELLOW, COLOR_BLACK);
  init_pair(2, COLOR_CYAN, COLOR_BLACK);
  init_pair(3, COLOR_WHITE, COLOR_BLUE);   // 단축키 이름 색상 쌍
  init_pair(4, COLOR_YELLOW, COLOR_BLUE);  //  단축키 색상 쌍
  init_pair(5, COLOR_RED, COLOR_WHITE);    //  불가능한 단축이름 색상 쌍
  init_pair(6, COLOR_BLACK, COLOR_WHITE);  //  불가능한 단축이름 색상 쌍
  init_pair(7, COLOR_WHITE, COLOR_CYAN);   //  다이얼로그 박스 사용자 입력 폼 색
  init_pair(8, COLOR_YELLOW,
            COLOR_BLUE);  //  다이얼로그 박스 정보문장 및 버튼 색상

  getCurrDir(&absDIRstr,
             &maxpath);  // 현재 디렉터리 얻어오기.
                         // absDIRstr에 동적 메모리가 할당되므로 해제 꼭하기~~

  refresh();  // stdscr에 대한 refresh를 해주고
              //  윈도우들을 wrefresh해줘야 제대로 나왔다.

  // ---------------- WINDOW 생성 및 설정 부 ------------------------------

  directory_display_win =
      newwin(3, COLS, 0, 0);  // 현재 디렉토리 표시 윈도우 생성.

  entity_list_win = newwin(LINES - 7, COLS, 3, 0);  // 개체 목록 윈도우 생성.
  keypad(entity_list_win, TRUE);

  entity_detail_display_win =
      newwin(1, COLS, LINES - 4, 0);  // 상세 정보 표시 윈도우 생성.

  shortcut_display_win = newwin(3, COLS, LINES - 3, 0);  // 기능키 윈도우 생성.

  dlg_win_mkdir = newwin(8, COLS - 20, LINES / 2 - 8, 10);
  // 디렉토리 생성 다이얼로그 박스.

  dlg_win_rename =
      newwin(8, COLS - 20, LINES / 2 - 8, 10);  // 이름 변경 다이얼로그 박스.

  dlg_win_chmod =
      newwin(10, COLS - 20, LINES / 2 - 8, 10);  // 권한 변경 다이얼로그 박스.

  //----------------------------------------------------------------------

  //------- 패널 설정 부 --------------------------------

  my_panels[0] = new_panel(entity_list_win);
  my_panels[1] = new_panel(dlg_win_mkdir);
  my_panels[2] = new_panel(dlg_win_rename);
  my_panels[3] = new_panel(dlg_win_chmod);

  set_panel_userptr(my_panels[0], my_panels[1]);
  set_panel_userptr(my_panels[1], my_panels[0]);
  set_panel_userptr(my_panels[0], my_panels[2]);
  set_panel_userptr(my_panels[2], my_panels[0]);
  set_panel_userptr(my_panels[0], my_panels[3]);
  set_panel_userptr(my_panels[3], my_panels[0]);

  top = my_panels[0];

  //-------------------------------------------------

  DirectoryWinDisplay(directory_display_win, absDIRstr);

  EntityListWinDisplay(
      entity_list_win);  // 개체 목록 윈도우 표시. // initMenu와 연관됨.

  ShortCutWinDisplay(shortcut_display_win);

  initMenu(&n_choices, &d_ptrArray, &my_items, &rtn_entity_size_chr,
           &entity_list_win, &my_menu, absDIRstr);

  wrefresh(entity_list_win);

  mark_val = 0;
  if (sigsetjmp(position, 1) == 0) {
    act.sa_handler = ctrlC_SIGINT;
    if (sigaction(SIGINT, &act, NULL) == -1) {
      endwin();
      perror("위치: main(), sigaction()오류");
      exit(-6);
    }
  }
  mark_val = 1;  // sigsetjmp()가 수행되었음을 표시하기 위해.

  while (1) {
    if (main_ipt_ch != 0x03)
      if (main_ipt_ch != KEY_F(12)) {
        main_ipt_ch = wgetch(entity_list_win);
      }

    if (main_ipt_ch == KEY_F(12)) {
      break;
    }  // 27: ESC를 사용할 경우 다른 키입력에서도 종료되는 경우가 있어
       //  F12로 변경

    switch (main_ipt_ch) {
      case KEY_DOWN:
        menu_driver(my_menu, REQ_DOWN_ITEM);
        // REQ_SCR_DPAGE 에 대해서는 나중에 보자.
        break;

      case KEY_UP:
        menu_driver(my_menu, REQ_UP_ITEM);
        break;

      case KEY_LEFT:
        menu_driver(my_menu, REQ_LEFT_ITEM);
        break;

      case KEY_RIGHT:
        menu_driver(my_menu, REQ_RIGHT_ITEM);
        break;

      case ' ':  // Space Bar 입력
        // 아이템 체크 및 선택바가 아래로 내려가도록 함.
        menu_driver(my_menu, REQ_TOGGLE_ITEM);
        menu_driver(my_menu, REQ_DOWN_ITEM);

        // 스페이스 바로 토글되면 그때의
        // 파일명을 어떤 배열에 보관해둬야 할 거 같다.
        // 나중에 다중 파일 복사/삭제/이동을 위해서 ...

        break;

      case 10:  // Enter 실행속성이 있는 파일에 한해서
                // exec()를 이용해 실행
                //
      {         // Enter 내부 블럭 시작
        ITEM *curItem;
        curItem = current_item(my_menu);
        int pce_rtn_val;
        char *entity_name = (char *)item_name(curItem);

        void (*p)(char *);  // 사용자 포인터와 연결해둔 함수를
                            // 사용하기 위한 변수.

        // 디렉토리인지 실행파일인지 판별
        pce_rtn_val = print_current_entity(item_name(curItem), 1, NULL);
        switch (pce_rtn_val) {
          case 1:
            // 선택된 아이템이 디렉토리일 경우.
            // 디렉토리를 바꿔야함.
            chdir(entity_name);

            // 디렉토리가 바뀌는 이벤트 에서 먼저 메모리 해제.
            freeDynamicMem(&my_menu, &my_items, &d_ptrArray,
                           &rtn_entity_size_chr, &absDIRstr, &n_choices);
            // 메모리 해제 끝..

            getCurrDir(&absDIRstr, &maxpath);
            DirectoryWinDisplay(directory_display_win, absDIRstr);

            // 현재 디렉터리 얻어오기.
            // absDIRstr에 동적 메모리가 할당되므로 나중에 해제 꼭하기~~

            // Initialize items
            initMenu(&n_choices, &d_ptrArray, &my_items, &rtn_entity_size_chr,
                     &entity_list_win, &my_menu, absDIRstr);
            break;

          case 2:  // 실행파일은 실행되어지게 함.
            p = item_userptr(curItem);

            execFilenameRev(entity_name, strlen(entity_name) + 1,
                            &exec_filename);

            p(exec_filename);

            free(exec_filename);  // 변환된 실행파일문자열 동적메모리 해제
            exec_filename = (char *)NULL;
            break;

        }  // 디렉토리 또는 파일 실행 switch end

      }  // Enter 내부 블럭 끝

      break;

      case 0x03:  // Copy 명령
        copy1move2 = 1;
        SelectedItemClipBoard(
            &selectItemCount[0], /* 선택된 아이템의 갯수 */
            absDIRstr,           /* 단축키가 눌려진 시점의 절대경로 */
            &my_items,           /* 아이템의 배열의 주소  */
            &my_menu,            /* 메뉴 포인터의 주소.  */
            &clip_items,         /* 아이템의 절대경로와 이름을 저장할 포인터 */
            &bakAbsDIRstr);
        mvwaddch(shortcut_display_win, 1, COLS - 2,
                 'C' | A_BOLD | A_BLINK | COLOR_PAIR(4));
        // SIGINT신호가 들어오면 ch에 0x03이 들어가도록 하자.

        main_ipt_ch = 0x00;
        break;

      case 0x0f:  // Move 명령
        copy1move2 = 2;
        SelectedItemClipBoard(
            &selectItemCount[0], /* 선택된 아이템의 갯수 */
            absDIRstr,           /* 단축키가 눌려진 시점의 절대경로 */
            &my_items,           /* 아이템의 배열의 주소  */
            &my_menu,            /* 메뉴 포인터의 주소.  */
            &clip_items,         /* 아이템의 절대경로와 이름을 저장할 포인터 */
            &bakAbsDIRstr);
        mvwaddch(shortcut_display_win, 1, COLS - 2,
                 'O' | A_BOLD | A_BLINK | COLOR_PAIR(4));

        break;

      case 0x04:  // Delete 명령
        // 선택다음 바로 삭제되도록 SelectedClipBoard()호출후
        // DeleteItemClipBoard 호출
        SelectedItemClipBoard(
            &selectItemCount[0], /* 선택된 아이템의 갯수 */
            absDIRstr,           /* 단축키가 눌려진 시점의 절대경로 */
            &my_items,           /* 아이템의 배열의 주소  */
            &my_menu,            /* 메뉴 포인터의 주소.  */
            &clip_items,         /* 아이템의 절대경로와 이름을 저장할 포인터 */
            &bakAbsDIRstr);

        DeleteItemClipBoard(&selectItemCount[0], &clip_items
                            /* 아이템의 절대경로와 이름을 저장할 포인터 */
        );

        // 디렉토리가 바뀌는 이벤트와 마찬가지로 메뉴 재생성.
        freeDynamicMem(&my_menu, &my_items, &d_ptrArray, &rtn_entity_size_chr,
                       (char **)NULL  //&absDIRstr
                       ,
                       &n_choices);
        // 메모리 해제 끝..

        // getCurrDir(&absDIRstr, &maxpath);
        // DirectoryWinDisplay(directory_display_win, absDIRstr);

        // Initialize items
        initMenu(&n_choices, &d_ptrArray, &my_items, &rtn_entity_size_chr,
                 &entity_list_win, &my_menu, absDIRstr);
        // 메뉴 재생성 끝

        mvwaddch(shortcut_display_win, 1, COLS - 2,
                 'D' | A_BOLD | A_BLINK | COLOR_PAIR(4));

        break;

      case 0x0b:  // Mkdir 명령

        Dlg_Mkdir(dlg_win_mkdir, absDIRstr, &usrIptName[0]);

        if (usrIptName[0] !=
            '\0')  // 취소 버튼을 눌렀다면 '\0'이므로 메뉴재생성필요없음
        {
          // 디렉토리가 바뀌는 이벤트와 마찬가지로 메뉴 재생성.
          freeDynamicMem(&my_menu, &my_items, &d_ptrArray, &rtn_entity_size_chr,
                         (char **)NULL  //&absDIRstr
                         ,
                         &n_choices);
          // 메모리 해제 끝..

          //  getCurrDir(&absDIRstr, &maxpath);
          //  DirectoryWinDisplay(directory_display_win, absDIRstr);

          // 현재 디렉터리 얻어오기.
          // absDIRstr에 동적 메모리가 할당되므로 나중에 해제 꼭하기~~

          // Initialize items
          initMenu(&n_choices, &d_ptrArray, &my_items, &rtn_entity_size_chr,
                   &entity_list_win, &my_menu, absDIRstr);
          // 메뉴 재생성 끝
        }
        top = (PANEL *)panel_userptr(top);
        top_panel(top);

        update_panels();
        mvwaddch(shortcut_display_win, 1, COLS - 2,
                 'K' | A_BOLD | A_BLINK | COLOR_PAIR(4));
        break;

      case 0x08:  // Chmod 명령
        SelectedItemClipBoard(
            &selectItemCount[0], /* 선택된 아이템의 갯수 */
            absDIRstr,           /* 단축키가 눌려진 시점의 절대경로 */
            &my_items,           /* 아이템의 배열의 주소  */
            &my_menu,            /* 메뉴 포인터의 주소.  */
            &clip_items,         /* 아이템의 절대경로와 이름을 저장할 포인터 */
            &bakAbsDIRstr);      // 다중 개체 권한 변경이 가능해야하므로 이 함수
                                 // 호출 후
        //

        // DeleteItemClipBoard(&selectItemCount[0],
        //     &clip_items
        /* 아이템의 절대경로와 이름을 저장할 포인터 */
        //  );
        Dlg_Chmod(dlg_win_chmod, &selectItemCount[0], &clip_items);

        top = (PANEL *)panel_userptr(top);
        top_panel(top);
        update_panels();

        mvwaddch(shortcut_display_win, 1, COLS - 2,
                 'H' | A_BOLD | A_BLINK | COLOR_PAIR(4));
        break;

      case 0x16:  // Paste 명령 (CentOS / Rocky에서 정상 동작)
      case 0x63:  // Ubuntu WSL 환경에서는 Ctrl + V가 0x63으로 들어왔다.
        // Paste명령이 시작되자 마자 Ctrl+C 시그널이 무시되어야함.
        act.sa_handler = SIG_IGN;
        if (sigaction(SIGINT, &act, &oldact) == -1)  // 이전 oldact에 저장
        {
          perror("위치:main(), sigaction()오류-무시저장");
          exit(-7);
        }
        mvwaddch(shortcut_display_win, 1, COLS - 2,
                 'V' | A_BOLD | A_BLINK | COLOR_PAIR(4));
        PasteItemClipBoard(
            &selectItemCount[0],
            /* 선택된 아이템의 갯수 배열  0번은 이전 1번은 현재 */
            absDIRstr,  /* Ctrl+V단축키가 눌려진 시점의 절대경로 */
            &clip_items /* 아이템의 절대경로와 이름이 저장된 포인터 */
        );
        if (copy1move2 == 2) {
          DeleteItemClipBoard(&selectItemCount[0], &clip_items
                              /* 아이템의 절대경로와 이름을 저장할 포인터 */
          );
          copy1move2 = 0;
        }

        // 디렉토리가 바뀌는 이벤트와 마찬가지로 메뉴 재생성.
        freeDynamicMem(&my_menu, &my_items, &d_ptrArray, &rtn_entity_size_chr,
                       (char **)NULL  //&absDIRstr
                       ,
                       &n_choices);
        // 메모리 해제 끝..

        // getCurrDir(&absDIRstr, &maxpath);
        // DirectoryWinDisplay(directory_display_win, absDIRstr);

        // Initialize items
        initMenu(&n_choices, &d_ptrArray, &my_items, &rtn_entity_size_chr,
                 &entity_list_win, &my_menu, absDIRstr);
        // 메뉴 재생성 끝
        //  다시 Ctrl+C가 활성화되도록 함.
        act = oldact;
        if (sigaction(SIGINT, &act, NULL) == -1) {
          perror("위치:main(), sigaction()오류-복구저장");
          exit(-8);
        }

        break;

      case 0x0e:  // Rename 명령
      {
        ITEM *curItem;
        curItem = current_item(my_menu);
        char *entity_name = (char *)item_name(curItem);

        Dlg_Rename(dlg_win_rename, entity_name, &usrIptName[0]);

        if (usrIptName[0] != '\0') {
          // 디렉토리가 바뀌는 이벤트와 마
          // 찬가지로 메뉴 재생성.
          freeDynamicMem(&my_menu, &my_items, &d_ptrArray, &rtn_entity_size_chr,
                         (char **)NULL  //&absDIRstr
                         ,
                         &n_choices);
          // 메모리 해제 끝..

          // getCurrDir(&absDIRstr, &maxpath);
          // DirectoryWinDisplay(directory_display_win, absDIRstr);

          // 절대경로가 바뀌지 않는 이벤트이므로 (char **)NULL을 전달하고
          // 현재 경로에 대한 메모리 해제나 다시 받아오는 작업은 없도록 했다.

          // Initialize items
          initMenu(&n_choices, &d_ptrArray, &my_items, &rtn_entity_size_chr,
                   &entity_list_win, &my_menu, absDIRstr);
          // 메뉴 재생성 끝
        }
      }

        top = (PANEL *)panel_userptr(top);
        top_panel(top);
        update_panels();

        mvwaddch(shortcut_display_win, 1, COLS - 2,
                 'N' | A_BOLD | A_BLINK | COLOR_PAIR(4));
        break;

      case 0x1f:  // Fast CD 명령
        mvwaddch(shortcut_display_win, 1, COLS - 2,
                 '/' | A_BOLD | A_BLINK | COLOR_PAIR(5));
        break;

    }  // switch end
    //----현재 메뉴가 가리키고 있는 개체 명에 대한 정보들을  불러와 출력-------
    // 파일 정보 [이름] - [크기] - [수정날짜] - [소유자.그룹]  - [권한] 을
    // 표시해야할 부분.
    {  // 내부 블럭 시작:/
      ITEM *curItem;
      curItem =
          current_item(my_menu);  // 현재 막대가 가리키고 있는 아이템 불러옴.

      wmove(entity_detail_display_win, 0, 2);
      wclrtoeol(entity_detail_display_win);

      wprintw(entity_detail_display_win, "[%s] ", item_name(curItem));

      print_current_entity(item_name(curItem), 0, entity_detail_display_win);

      wrefresh(entity_detail_display_win);

      wrefresh(shortcut_display_win);

      pos_menu_cursor(
          my_menu);  // 개체 정보 출력으로 옮겨진 커서를 다시 메뉴로...

    }  // 내부 블럭 끝.
    //-------------------------------------------------------------------------
    // wrefresh(entity_list_win);

    top = my_panels[0];  // entity_list_win가 맨 위로 표시되게
    top_panel(top);

    update_panels();
    doupdate();

  }  // while end

  // 할당된 동적 메모리 해제하기...

  freeDynamicMem(&my_menu, &my_items, &d_ptrArray, &rtn_entity_size_chr,
                 &absDIRstr, &n_choices);

  if (bakAbsDIRstr != NULL) {
    free(bakAbsDIRstr);
    bakAbsDIRstr = (char *)NULL;
  }

  // 패널을 먼저 해제 (윈도우보다 먼저!)
  del_panel(my_panels[0]);
  del_panel(my_panels[1]);
  del_panel(my_panels[2]);
  del_panel(my_panels[3]);

  // 윈도우를 나중에 해제
  delwin(directory_display_win);
  delwin(entity_list_win);  // WINDOW를 메모리에서 해제 그러나 이미 출력된
                            // 이미지가 제거되지는 않음
  delwin(entity_detail_display_win);  // 상세정보 윈도우 메모리 해제
  delwin(shortcut_display_win);       // 단축키 윈도우 메모리 해제
  delwin(dlg_win_mkdir);
  delwin(dlg_win_rename);
  delwin(dlg_win_chmod);

  // 메모리 해제 끝..

  endwin();

  unsetenv("NCURSES_NO_UTF8_ACS");  // 설정한 환경변수 해제

  return 0;
}  // main() end

void print_in_middle(WINDOW *win, int starty, int startx, int width,
                     char *string, chtype color) {
  int length, x, y;
  float temp;

  if (win == NULL) win = stdscr;
  getyx(win, y, x);
  if (startx != 0) x = startx;
  if (starty != 0) y = starty;
  if (width == 0) width = COLS;

  length = strlen(string);
  temp = (width - length) / 2;
  x = startx + (int)temp;
  wattron(win, color);
  mvwprintw(win, y, x, "%s", string);
  wattroff(win, color);

}  // print_in_middle end

/* 파일처리론과제 #3-2에서 했던 myls를 약간 수정 */
int print_current_entity(const char *entity_name, const int pce_mode,
                         WINDOW *entity_detail_display_win) {
  // pec_mode 값에 따라 작동 방식을 약간 다르게.
  //
  // 0 이면 전체 실행
  //   : 개체의 전체 정보를 중간 부분에 출력
  //   : 반환 값은 0이지만 사용하지 않음.
  //
  // 1 이면 부분 실행 -
  //   : 현재 개체가 디렉토리인지 검사 후
  //   : 만족할 경우 반환값 1
  //   : 아닐경우 반환값 0
  //

  int i;  // 권한 검사를 위한 for문에서 사용.

  struct stat statbuf;  // 파일모드 정보들을 저장할 stat형의 구조체 선언.

  char file_attribute;  // 파일의 속성을 저장할 문자변수.

  const char file_perm_templet[10] =
      "rwxrwxrwx";  // 파일권한 문자들의 틀을 위한 상수
  char view_perm[10];

  const short octperm[] = {S_IRUSR, S_IWUSR, S_IXUSR, S_IRGRP, S_IWGRP,
                           S_IXGRP, S_IROTH, S_IWOTH, S_IXOTH};

  struct tm *tp_time;  // tm 구조체 사용;
  time_t timeval_time;

  struct group *grpPtr;
  struct passwd *usrPtr;

  if (lstat(entity_name, &statbuf) == -1) {
    if (errno == 75) {
      // 개체 크기가 2기가를 넘을 때 발생하는 오류
      if (pce_mode == 0)
        printw("2GB 이상의 파일은 정보를 읽어올 수 없는 문제가 있습니다.");
      return 0;
    }

    endwin();
    fprintf(stderr,
            "(오류번호: %d) - \"%s\" 파일명을 인수로 lstat함수를 사용할 때"
            "오류가 발생했습니다.\n",
            errno, entity_name);
    perror(" ");
    exit(-9);
  }

  if (S_ISREG(statbuf.st_mode) == 1) {  // 일반 파일
    file_attribute = '-';
  } else if (S_ISSOCK(statbuf.st_mode)) {  // 소켓
    file_attribute = 's';
  } else if (S_ISBLK(statbuf.st_mode)) {  // block special filei; 블록 특수 파일
    file_attribute = 'b';
  } else if (S_ISCHR(
                 statbuf.st_mode)) {  // character special file; 문자 특수 파일
    file_attribute = 'c';
  } else if (S_ISDIR(statbuf.st_mode)) {  // 디렉토리
    file_attribute = 'd';
  } else if (S_ISFIFO(statbuf.st_mode)) {  // 파이프 또는 FIFO
    file_attribute = 'f';
  } else if (S_ISLNK(statbuf.st_mode)) {  // 심볼릭 링크
    file_attribute = 'l';
  } else {
    file_attribute = '?';
  }

  for (i = 0; i < 9; i++) {
    if (statbuf.st_mode &
        octperm[i]) {  // 비트별 AND를 사용하여 허가가 설정되었는지 테스트
      view_perm[i] = file_perm_templet[i];
    } else {
      view_perm[i] = '-';
    }
  }

  view_perm[9] = '\0';
  // 1)교제 75쪽의 내용 거의 그대로 작성.

  if (pce_mode == 1) {
    if (file_attribute == 'd') {
      return 1;
    } else if (access(entity_name, X_OK) == 0) {
      return 2;
    } else {
      return 0;
    }
  }  // 모드 선택 if end

  // 1)교제 433쪽 12.4 시간 참고

  timeval_time = statbuf.st_atime;  // 최종 수정시간.
  tp_time = localtime(&timeval_time);

  if ((usrPtr = getpwuid(statbuf.st_uid)) == NULL) {
    endwin();
    perror("UID에 대한 구조체를 받을 때 오류.");
    exit(-10);
  }

  if ((grpPtr = getgrgid(statbuf.st_gid)) == NULL) {
    endwin();
    perror("GID에 대한 구조체를 받을 때 오류. ");
    exit(-11);
  }

  if (entity_detail_display_win == NULL) {
    return 0;
  }
  wprintw(entity_detail_display_win,
          "[%ld Byte] [%s.%s] [%c%s] [%04d/%02d/%02d-%02d:%02d]",
          statbuf.st_size  // 바이트 단위 크기 (정규 파일일때.)
          ,
          usrPtr->pw_name, grpPtr->gr_name, file_attribute, view_perm,
          (tp_time->tm_year) + 1900, (tp_time->tm_mon) + 1, tp_time->tm_mday,
          tp_time->tm_hour, tp_time->tm_min);

  return 0;

}  // print_current_entity() end

void entity_size_calc(const char *entity_name,
                      char *rtn_entity_size_chr) {  // 개체의 이름을 입력 받으면
                                                    // 크기를 계산하는 함수.

  const size_t Byte = 1024;         // 바이트 상수
  const size_t MegaB = 1048576;     // 메가바이트 상수
  const size_t GigaB = 1073741824;  // 기가바이트 상수

  struct stat statbuf;  // 파일모드 정보들을 저장할 stat형의 구조체 선언.

  size_t entity_size;
  char char_rlt;
  unsigned int ptsn_size[2];  // 0번원소는 소수점위 1번원소는 소수점 아래 표현

  if (lstat(entity_name, &statbuf) == -1) {
    if (errno ==
        75)  // lstat()가 2GB이상의 크기의 파일을 읽을 때 발생하는 오류번호
    {        // 해결책 못찾음.
      sprintf(rtn_entity_size_chr, "2GB<?");
      return;
    }
    endwin();
    fprintf(stderr,
            "(오류번호: %d) - \"%s\" 파일명을 인수로 lstat함수를 사용할 때"
            "오류가 발생했습니다.\n",
            errno, entity_name);
    perror(" ");
    exit(-12);
  }

  entity_size = statbuf.st_size;

  if (entity_size >= GigaB) {
    ptsn_size[0] = entity_size / GigaB;
    ptsn_size[1] = entity_size % GigaB;

    if (ptsn_size[1] > 0 && ptsn_size[1] < (GigaB / 10)) {
      ptsn_size[1] = 1;  // .0에서 조금이라도 넘으면 1로 표현하고 싶어서
    } else {
      ptsn_size[1] = ptsn_size[1] / (GigaB / 10);
    }
    char_rlt = 'G';

    entity_size = ptsn_size[0];
  } else if (entity_size >= MegaB) {
    ptsn_size[0] = entity_size / MegaB;
    ptsn_size[1] = entity_size % MegaB;

    if (ptsn_size[1] > 0 && ptsn_size[1] < (MegaB / 10)) {
      ptsn_size[1] = 1;
    } else {
      ptsn_size[1] = ptsn_size[1] / (MegaB / 10);
    }
    char_rlt = 'M';

    entity_size = ptsn_size[0];
  } else if (entity_size >= Byte) {
    ptsn_size[0] = entity_size / Byte;
    ptsn_size[1] = entity_size % Byte;

    if (ptsn_size[1] > 0 && ptsn_size[1] < (Byte / 10)) {
      ptsn_size[1] = 1;
    } else {
      ptsn_size[1] = ptsn_size[1] / (Byte / 10);
    }
    char_rlt = 'K';

    entity_size = ptsn_size[0];
  } else {
    ptsn_size[0] = entity_size;
    ptsn_size[1] = 0;
    char_rlt = 'B';
  }

  if (ptsn_size[1] == 10) {
    --ptsn_size[1];
  }
  // atoi()의 기능을 반대로 사용하고 싶을 때. sprintf()사용
  sprintf(rtn_entity_size_chr, "%d.%d%c", ptsn_size[0], ptsn_size[1], char_rlt);
}

void getCurrDir(char **absDIRstr, long *maxpath) {
  // 프로그램을 실행할 때 최대 경로길이의 실제값을 정하기 위해서. 2)교제 p205
  if ((*maxpath = pathconf(".", _PC_PATH_MAX)) == -1) {
    endwin();
    fprintf(stderr, "경로이름 길이를  결정하는데 실패하였습니다\n.");
    perror(" ");
    exit(-13);
  }

  if ((*absDIRstr = (char *)malloc(*maxpath)) == NULL) {
    endwin();
    perror("경로이름을 위한 공간을 할당하는데 실패하였습니다.");
    exit(-14);
  }

  if (getcwd(*absDIRstr, *maxpath) == NULL) {
    endwin();
    fprintf(stderr, "현재 작업 디렉토리를 열수 없습니다.\n");
    perror(" ");
    exit(-15);
  }

}  // getCurrDir() End

// 디렉토리가 아닌 실행속성을 가진 파일이 선택된 상태에서 엔터키를 입력했을 때,
// 실행되야하는 함수를 정의함.
//
// 아이템에 대한 사용자 포인터를 미리 저장하고 사용하도록 하자. 웹1) Example 24

void execFunc(const char *filename)  // 실행파일 선택 상태에서 엔터 이벤트가
                                     // 일어났을 때 수행하는 함수.
{
  int status;
  pid_t rtn_fork;

  // sigset_t mask, oldmask;
  // 새로 값을 지정할 마스크와 이전 마스크를 저장할 변수. - 교제2) 342쪽

  struct sigaction act_exec, oldact;

  char pause_msg[] = "프로그램 종료됨. \n[Enter]키를 눌러주세요...\n";

  char *const cmdlist[2] = {(char *)filename,
                            (char *)0};  // execvp틀에 맞추기 위해서.

  // 프로그램을 실행시키기 위해서 잠시 curses모드를 나갈 필요가 있음.
  // 웹1) Example 12, man 페이지 참고.

  def_prog_mode();  // 현재 터미널의 curses모드 상태를 저장

  endwin();  // 임시적으로 curses mode 종료.

  act_exec.sa_handler = SIG_IGN;
  if (sigaction(SIGINT, &act_exec, &oldact) == -1) {
    perror("위치:execFunc(), sigaction()오류");
    exit(-16);
  }

  rtn_fork = fork();
  switch (rtn_fork)  // 하나의 자식만 만듦 .. 단순 엔터에서 추가 인수넣는 것을
                     // 고려하지 않음.
  {
    case -1:  // fork오류
      perror("위치: execFunc() - fork()오류");
      exit(-17);
      break;
    case 0:  // 자식 프로세스

      act_exec.sa_handler = SIG_DFL;
      if (sigaction(SIGINT, &act_exec, NULL) == -1) {
        perror("위치:execFunc(), sigaction()오류");
        exit(-18);
      }  // 자식은 SIGINT가 기본 값으로 실행되게.

      execvp(cmdlist[0], &cmdlist[0]);
      perror("위치: execFunc() - execvp()오류");
      exit(-19);
  }  // switch end;

  wait(&status);

  write(STDOUT_FILENO, filename,
        strlen(filename) + 1);  // filename은 char* 이니 sizeof를 사용하면X
  write(STDOUT_FILENO, pause_msg, sizeof(pause_msg));
  // printf()출력이 바로 작동하지 않아 write()사용..

  getchar();  // 자식프로그램 종료후 키보드 입력 대기.

  act_exec =
      oldact;  // 자식 프로세스가 실행되는 동안 Ctrl + C 가 입력 되었을 때
  // 자식은 그것을 SIGINT로 정상 인식해야하고 부모는 무시했다가
  if (sigaction(SIGINT, &act_exec, NULL) == -1) {
    perror("위치:execFunc(), sigaction()오류");
    exit(-20);
  }
  // 자식프로세스의 실행이 끝나면 예전 설정으로 복귀한다.
  // 다시 SIGINT입력시 void ctrlC_SIGINT(int signo) 이벤트 핸들러 함수가
  // 실행되도록 한다.

  reset_prog_mode();  // 앞의 def_prog_mode()가 curses모드일 때를 저장했으므로
                      // curses mode를 다시 시작 함.

}  // execFunc() end

void execFilenameRev(const char *filename, const int name_num,
                     char **exec_filename) {
  // execvp() 로 실행파일 a.out 파일명 스트링을 그대로 보내면
  // PATH환경변수에 "." 이 포함되어있지않은 환경에서는 명령을 실행할 수 없음.
  // 실행파일명 앞부분에 "./" 문자열을 덧붙일 목적의 함수.

  // name_num은 \0을 포함한 수로 생각함.

  int i;

  if ((*exec_filename = (char *)malloc(sizeof(char) * (name_num))) == NULL) {
    perror("위치:execFilenameRev() - malloc()오류");
    return;
  }

  // 문자열 두칸씩 밀기.
  for (i = 0; i < name_num; i++) {
    (*exec_filename)[name_num - i + 1] = filename[name_num - i - 1];
  }

  (*exec_filename)[0] = '.';
  (*exec_filename)[1] = '/';

}  // execFilenameRev() end

void ctrlC_SIGINT(int signo)  // SIGINT 시그널 발생시 작동함수.
{
  if (mark_val != 1)  // 의도하지 않은 신호이므로 무시.  교제3) 393쪽
    return;

  // printw("ctrlC_SIGINT()  signo=%d", signo);
  if (signo == 2) main_ipt_ch = 0x03;
  // raw()모드에서 0x03이 CTRL+C를 의미

  siglongjmp(position, 1);
}  // ctrlC_SIGINT() end

void ShortCutWinDisplay(WINDOW *shortcut_display_win)  // 단축키를 표시할 윈도우
{
  int win_x;  // win_y 제거 (사용하지 않으므로)

  char botLabelCtrl[] = {"Ctrl+"};
  char botLabelCopy[] = {"Copy( )"};
  char botLabelMove[] = {"Move( )"};
  char botLabelPaste[] = {"Paste( )"};
  char botLabelRename[] = {"Rename( )"};
  char botLabelChmod[] = {"Chmod( )"};
  char botLabelMkdir[] = {"Mkdir( )"};
  char botLabelDel[] = {"Delete( )"};
  char botLabelFMove[] = {"[ ]"};

  char botLabelKey[] = {'c', 'o', 'v', 'n', 'h', 'k', 'd', '/'};
  // Move의 단축키를 m으로 하고 싶었는데, ENTER입력과 중복되서
  // 일단은 o로 바꿔둠.
  char *botLabel[9] = {
      &botLabelCtrl[0],  &botLabelCopy[0],   &botLabelMove[0],
      &botLabelPaste[0], &botLabelRename[0], &botLabelChmod[0],
      &botLabelMkdir[0], &botLabelDel[0],    &botLabelFMove[0]};
  int label_x;  // 기능키라벨의 시작좌표
  int i;

  win_x = getbegx(shortcut_display_win);  // X 좌표만 가져오기

  label_x = win_x + 2;  // 라벨 시작 x좌표를 2로 시작
  box(shortcut_display_win, 0, 0);

  wattron(shortcut_display_win, COLOR_PAIR(3) | A_BOLD);

  mvwaddstr(shortcut_display_win, 1, label_x, botLabel[0]);
  for (i = 1; i < 9; i++) {
    if (i == 8)  // 8번 빠른 디렉토리 이동 기능 아직 구현 안됨.
    {
      wattroff(shortcut_display_win, COLOR_PAIR(3) | A_BOLD);
      wattron(shortcut_display_win, COLOR_PAIR(5) | A_BOLD);

      label_x = (label_x + strlen(botLabel[i - 1]) + 1);

      mvwaddstr(shortcut_display_win, 1, label_x, botLabel[i]);
      mvwaddch(shortcut_display_win, 1, label_x + strlen(botLabel[i]) - 2,
               botLabelKey[i - 1] | COLOR_PAIR(6) | A_BOLD | A_UNDERLINE);

      wattroff(shortcut_display_win, COLOR_PAIR(5) | A_BOLD);
      wattron(shortcut_display_win, COLOR_PAIR(3) | A_BOLD);
      continue;
    }

    label_x = (label_x + strlen(botLabel[i - 1]) + 1);
    // strlen()에서 UTF-8의 한글 한글자 반환이 3BYTE이므로 이점을 고려해야함.
    // 이 부분이 제대로 되면 그냥 한글 사용하려고 했는데.
    // 작품 판넬에 낸 것처럼 한글의 좌표위치를 일일이 지정해주는 것은 문제가
    // 있다고 판단하여 영문으로 바꿔봄.

    mvwaddstr(shortcut_display_win, 1, label_x, botLabel[i]);
    mvwaddch(shortcut_display_win, 1, label_x + strlen(botLabel[i]) - 2,
             botLabelKey[i - 1] | COLOR_PAIR(4) | A_BOLD | A_UNDERLINE);
  }

  wattroff(shortcut_display_win, COLOR_PAIR(3) | A_BOLD);
  wrefresh(shortcut_display_win);
}  // ShortCutWinDisplay() end

void DirectoryWinDisplay(WINDOW *directory_display_win, const char *absDIRstr) {
  box(directory_display_win, 0, 0);

  wmove(directory_display_win, 1, 2);
  wprintw(directory_display_win, "현재 디렉토리 - ");

  wclrtoeol(directory_display_win);
  wattron(directory_display_win, A_BOLD | COLOR_PAIR(2));
  wprintw(directory_display_win, "%s", absDIRstr);
  wattroff(directory_display_win, A_BOLD | COLOR_PAIR(2));
  mvwaddch(directory_display_win, 1, COLS - 1, ACS_VLINE);

  wrefresh(directory_display_win);
}  // DirectoryWinDisplay() end

void EntityListWinDisplay(WINDOW *entity_list_win) {
  int max_x;  // max_y 제거 (사용하지 않으므로)

  // 개체 리스트 메뉴를 붙일 윈도우 생성.

  max_x = getmaxx(entity_list_win);  // X 좌표만 가져오기
  // 결국 우측끝 최하단 좌표.
  box(entity_list_win, 0, 0);
  print_in_middle(entity_list_win, 1, 0, max_x, "개체 이름 / 크기",
                  COLOR_PAIR(1) | A_BOLD);

  // 목록 윈도우 ESC키 문자 구분
  mvwaddch(entity_list_win, 0, max_x - 14, ACS_TTEE);  // ㅜ 문자 삽입
  mvwaddch(entity_list_win, 1, max_x - 14, ACS_VLINE);
  mvwprintw(entity_list_win, 1, max_x - 12, "종료:");

  wattron(entity_list_win, A_BOLD | COLOR_PAIR(4));
  wprintw(entity_list_win, "[F12]");
  wattroff(entity_list_win, A_BOLD | COLOR_PAIR(4));

  // window안에 ㅏ-----ㅓ 삽입
  mvwaddch(entity_list_win, 2, 0, ACS_LTEE);
  mvwhline(entity_list_win, 2, 1, ACS_HLINE, max_x - 2);
  mvwaddch(entity_list_win, 2, max_x - 1, ACS_RTEE);

  mvwaddch(entity_list_win, 2, max_x - 14,
           ACS_BTEE);  // ESC구분자 ㅗ  문자 삽입

  wrefresh(entity_list_win);
}  // EntityListWinDisplay() end

// 다중 선택에 대응되어야 하는 Copy, Move, Rename, Chmod, Delete 함수에 필요한
// 다중선택된 개체들의 전체 경로, 파일명|디렉토리명을 메모리에 보관하는 함수
// 구현

void SelectedItemClipBoard(
    int selectItemCount[], /* 선택된 아이템의 갯수배열  0번은 이전 1번은 현재 */
    char *absDIRstr,       /* Ctrl+C 단축키가 눌려진 시점의 절대경로 */
    ITEM ***my_items,      /* 아이템의 배열의 주소  */
    MENU **my_menu,        /* 메뉴 포인터의 주소.  */
    CLIP_ENTITY **clip_items, /* 아이템의 절대경로와 이름을 저장할 포인터 */
    char **bakAbsDIRstr       /*단축키가 눌려진 시점의 절대경로 복사본*/
) {
  int i, j;
  char *name_ptr_tmp;
  int etc_num;
  int AllItemNum;

  selectItemCount[1] = 0;  // 현재의 선택 카운트 초기화
  AllItemNum = item_count(*my_menu);

  for (i = 0; i < AllItemNum; i++)  // 전체 아이템의 갯수 만큼 반복
  {
    if (item_value((*my_items)[i]) == TRUE)  // 아이템이 선택되었는지 검사.
    {
      (selectItemCount[1])++;
    }
  }  // (*clip_tiems) 의 크기를 결정하기 위해 반복을 나눌 수 밖에 없을 것 같다.

  if (selectItemCount[0] == 0 && selectItemCount[1] == 0) {
    return;  // 선택한 아이템이 없다면 함수 끝냄.
  }

  // 동적메모리를 할당할 구조체의 포인터가 NULL이 아닐 때는 메모리를 해제하자.
  if (*clip_items != (CLIP_ENTITY *)NULL) {
    for (i = 0; i < selectItemCount[0];
         i++)  // 과거의 갯수가 저장되있는 것을 메모리 해제.
    {
      free((char *)((*clip_items)[i].name));
      (*clip_items)[i].name = (char *)NULL;
    }

    free((*clip_items)[0].abspath);
    (*clip_items)[0].abspath = (char *)NULL;

    free((CLIP_ENTITY *)(*clip_items));
    (*clip_items) = (CLIP_ENTITY *)NULL;

    selectItemCount[0] = 0;  // 과거의 갯수 초기화
  }  // if free end

  if (*bakAbsDIRstr != (char *)NULL) {
    free((char *)(*bakAbsDIRstr));
    *bakAbsDIRstr = (char *)NULL;
  }

  etc_num = strlen(absDIRstr) + 1;

  if ((*bakAbsDIRstr = (char *)malloc(etc_num * sizeof(char))) == NULL) {
    endwin();
    perror(
        "malloc()오류 - 위치: SelectedItemClipBoard() -"
        "절대경로 백업저장 공간 동적 메모리 할당하는 부분.");
    exit(-21);
  }

  strncpy((*bakAbsDIRstr), absDIRstr, strlen(absDIRstr) + 1);

  if ((*clip_items = (CLIP_ENTITY *)malloc(sizeof(CLIP_ENTITY) *
                                           (selectItemCount[1]))) == NULL) {
    endwin();
    perror(
        "malloc()오류 - 위치: SelectedItemClipBoard() -"
        "개체목록을 복사해둘 구조체 포인터에 동적메모리 할당하는 부분.");
    exit(-22);
  }

  j = 0;
  for (i = 0; i < AllItemNum; i++)  // 전체 아이템의 갯수 만큼 반복
  {
    if (item_value((*my_items)[i]) == TRUE)  // 아이템이 선택되었는지 검사.
    {
      etc_num = strlen(name_ptr_tmp = (char *)item_name((*my_items)[i]));

      if (((*clip_items)[j].name =
               (char *)malloc((etc_num + 1) * sizeof(char))) == NULL) {
        endwin();
        perror(
            "malloc()오류 - 위치: SelectedItemClipBoard() -"
            "[개체목록.이름] 포인터에 동적메모리 할당하는 부분.");
        exit(-23);
      }

      strncpy((*clip_items)[j].name, name_ptr_tmp, etc_num);
      ((*clip_items)[j].name)[etc_num] = '\0';

      (*clip_items)[j].abspath = *bakAbsDIRstr;  // 절대경로 정보 추가.

      j++;
    }
  }
  // j값이 유지 되어야한다. 아래서 다시 이용.

  etc_num = strlen(*bakAbsDIRstr) + 1;

  if (((*clip_items)[0].abspath = (char *)malloc(sizeof(char) * etc_num)) ==
      NULL) {
    endwin();
    perror(
        "malloc()오류 - 위치: SelectedItemClipBoard() -"
        "[개체목록[0].절대경로] 포인터에 동적메모리 할당하는 부분.");
    exit(-23);  // 절대경로를 0번에 다만 복사해두고 나머지는 주소만 연관시킴.
  }  // 같은 디렉토리의 파일만 복사가 가능하기 때문에 아직은 여기까지...

  strncpy((*clip_items)[0].abspath, *bakAbsDIRstr, etc_num);

  for (i = 1; i < j; i++) {
    (*clip_items)[i].abspath = (*clip_items)[0].abspath;  // 절대경로 정보 추가.
  }

  selectItemCount[0] =
      selectItemCount[1];  // 메모리할당이 끝난후 현재의 갯수를 과거에 할당.

  return;

}  // SelectedItemClipBoard() end

// 파일처리론 과제 2-1,2 참조.
void PasteItemClipBoard(
    int selectItemCount[],   /* 선택된 아이템의 갯수 배열  0번은 이전 1번은 현재
                              */
    char *absDIRstr,         /* Ctrl+V단축키가 눌려진 시점의 절대경로 */
    CLIP_ENTITY **clip_items /* 아이템의 절대경로와 이름을 저장할 포인터 */
) {
  int i;
  char *src_full_path_name;  // [원본]경로를 포함한 전체 이름.
  char *des_full_path_name;  // [목적]경로를 포함한 전체 이름.

  int srcfd, desfd;  // 원본 파일 기술자, 목적지 파일 기술자.

  ssize_t nread;         // 읽은 바이트 수를 저장할 공간.
  char buffer[BUFSIZE];  // char를 문자로 생각하기보단 1바이트로 생각하자.

  struct stat statbuf;    // 파일모드 정보들을 저장할 stat형의 구조체 선언.
  mode_t perm = 0000000;  // 권한 초기값

  src_full_path_name = (char *)NULL;
  des_full_path_name = (char *)NULL;

  if (*clip_items == NULL ||
      selectItemCount[1] == 0)  // 현재 선택된 아이탬이 0이거나
  {                             // 구조체 포인터가 NULL이면 바로 끝냄.
    return;
  }

  for (i = 0; i < selectItemCount[1]; i++) {
    if ((src_full_path_name = (char *)malloc(
             sizeof(char) * (strlen((*clip_items)[i].name) +
                             strlen((*clip_items)[i].abspath) + 2))) == NULL) {
      endwin();
      perror(
          "malloc()오류 - 위치: PasteItemClipBoard() -"
          "원본파일 전체경로 문자열  포인터에 동적메모리 할당하는 부분.");
      exit(-24);
    }  // if end

    // '\0'과 파일과 절대경로 사이에 '/'하나를 넣기 위해서. + 2
    if ((des_full_path_name =
             (char *)malloc(sizeof(char) * (strlen((*clip_items)[i].name) +
                                            strlen(absDIRstr) + 2))) == NULL) {
      endwin();
      perror(
          "malloc()오류 - 위치: PasteItemClipBoard() -"
          "목적파일 전체경로 문자열  포인터에 동적메모리 할당하는 부분.");
      exit(-25);
    }

    sprintf(src_full_path_name, "%s/%s", (*clip_items)[i].abspath,
            (*clip_items)[i].name);

    sprintf(des_full_path_name, "%s/%s", absDIRstr, (*clip_items)[i].name);

    if (lstat(src_full_path_name, &statbuf) == -1) {
      perror(" ");
      free(src_full_path_name);
      src_full_path_name = (char *)NULL;
      free(des_full_path_name);
      des_full_path_name = (char *)NULL;
      continue;
    }

    perm = statbuf.st_mode;
    // 원래 원본파일의 권한을 그대로 얻어오기 위해서..

    if ((srcfd = open(src_full_path_name, O_RDONLY)) == -1) {
      // perror("오류: open() 원본파일 / 위치: PasteItemClipBoard() ");
      free(src_full_path_name);
      src_full_path_name = (char *)NULL;
      free(des_full_path_name);
      des_full_path_name = (char *)NULL;
      continue;
    }

    if ((desfd = open(des_full_path_name,
                      O_WRONLY           /* 쓰기전용으로 연다.  */
                          | O_CREAT      /* 파일이 존재하지 않으면 새로 생성. */
                          /* | O_TRUNC*/ /* 이미 존재하는 파일을 [R/W]연다면
                                            크기를 0으로 만듦. */
                          | O_EXCL /* 파일이 이미 존재하는 경우 오류 발생.  */
                      ,
                      perm /* 만들어질 파일의 권한 대입. */
                      )) == -1) {
      // perror("오류: open() 목적파일 / 위치: PasteItemClipBoard() ");
      close(srcfd);
      free(src_full_path_name);
      src_full_path_name = (char *)NULL;
      free(des_full_path_name);
      des_full_path_name = (char *)NULL;

      continue;
    } /* 목적지를 O_TRUNC로 해두면 한경로에서 선택후 복사명령으로 클립보드에
         붙힌후 경로를 바꿨다가 원래자리로 돌아온후 붙혀넣으면 복사할 파일의
         크기를 0으로 만들어버리는 문제가 있다.
        */

    while ((nread = read(srcfd, buffer, BUFSIZE)) > 0) {
      if (write(desfd, buffer, nread) <
          nread) {  // write()로 쓴 바이트 수가 nread와 같아야 정상인데,
        // 다르다면 오류...
        close(srcfd);
        close(desfd);
        exit(-26);
      }
    }

    close(srcfd);
    close(desfd);

    if (nread == -1) {
      exit(-27);  // 마지막 읽기에서 오류 발생.
    }

    free(src_full_path_name);
    src_full_path_name = (char *)NULL;
    free(des_full_path_name);
    des_full_path_name = (char *)NULL;
  }

}  // PasteItemClipBoard() end

void DeleteItemClipBoard(
    int selectItemCount[],   /* 선택된 아이템의 갯수 배열  0번은 이전 1번은 현재
                              */
    CLIP_ENTITY **clip_items /* 아이템의 절대경로와 이름을 저장할 포인터 */
) {
  int i;
  char *full_path_name;  // 경로를 포함한 전체 이름.

  full_path_name = (char *)NULL;

  if (*clip_items == (CLIP_ENTITY *)NULL ||
      selectItemCount[1] == 0)  // 현재 선택된 아이탬이 0이거나
  {                             // 구조체 포인터가 NULL이면 바로 끝냄.
    return;
  }

  for (i = 0; i < selectItemCount[1]; i++) {
    if ((full_path_name = (char *)malloc(
             sizeof(char) * (strlen((*clip_items)[i].name) +
                             strlen((*clip_items)[i].abspath) + 2))) == NULL) {
      endwin();
      perror(
          "malloc()오류 - 위치: DeleteItemClipBoard() -"
          "삭제할 개체 이름 앞에 절대경로를 붙이기 위한 메모리할당 부분.");
      exit(-28);

    }  // if end

    sprintf(full_path_name, "%s/%s", (*clip_items)[i].abspath,
            (*clip_items)[i].name);

    // stdio.h에서 제공하는 C라이브러리 함수. // Dialog박스를 출력해야하는데..
    // 아직안되서.
    remove(full_path_name);  // 디렉토리 및 파일 삭제를 할 수 있으므로 좀 위험한
                             // 것 같다.
    free(full_path_name);
    full_path_name = (char *)NULL;
  }

  for (i = 0; i < selectItemCount[1];
       i++)  // 과거의 갯수가 저장되있는 것을 메모리 해제
  {          // 하는 것이 아닌. 방금 지운 것에대한 것 해재 이므로
    // 현재가 낫겠다.
    free((char *)((*clip_items)[i].name));
    (*clip_items)[i].name = (char *)NULL;
  }

  free((char *)((*clip_items)[0].abspath));
  (*clip_items)[0].abspath = (char *)NULL;

  free((CLIP_ENTITY *)(*clip_items));
  (*clip_items) = (CLIP_ENTITY *)NULL;

  selectItemCount[0] = selectItemCount[1] = 0;

}  // DeleteItemClipBoard() end

//--- scandir()을 사용하는 initMenu() 안에서만 사용할 함수
//--------------------------
int dot_no_select(const struct dirent *entry)  //  "." 제외 목적의 filter함수.,
{
  if (strcmp(entry->d_name, ".") == 0)
    return 0;
  else
    return 1;
}

int ddot_no_select(
    const struct dirent *entry)  // "." ".." 제외 목적의 filter함수.,
{
  if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
    return 0;
  else
    return 1;
}
//----------------------------------------------------------------------------------

void Dlg_Mkdir(WINDOW *dlg_win_mkdir, const char *curDirName,
               char *usrIptName) {
  int x, max_x;  // y, max_y 제거 (사용하지 않으므로)
  int i;
  int attr_style;

  struct stat statbuf;    // 파일모드 정보들을 저장할 stat형의 구조체 선언.
  mode_t perm = 0000000;  // 권한 초기값

  chtype ipt;

  ipt = 0;
  usrIptName[0] = '\0';
  keypad(dlg_win_mkdir, TRUE);  // initscr()이전의 이 함수 호출은 의미없음.

  max_x = getmaxx(dlg_win_mkdir);  // X 좌표만 가져오기

  box(dlg_win_mkdir, 0, 0);

  wattron(dlg_win_mkdir, A_BOLD);
  mvwprintw(dlg_win_mkdir, 0, 5, " 디렉토리 생성 (Mkdir)");
  wattroff(dlg_win_mkdir, A_BOLD);

  wattron(dlg_win_mkdir, A_BOLD | COLOR_PAIR(8));
  mvwprintw(dlg_win_mkdir, 2, 3, " 현재 디렉토리      ");

  mvwprintw(dlg_win_mkdir, 4, 3, " 만들 디렉터리 이름 ");

  mvwprintw(dlg_win_mkdir, 6, 24, "  확 인  ");  // 버튼 1

  mvwprintw(dlg_win_mkdir, 6, 40, "  취 소  ");  // 버튼 2
  wattroff(dlg_win_mkdir, A_BOLD | COLOR_PAIR(8));

  attr_style = A_BOLD | COLOR_PAIR(7);
  wmove(dlg_win_mkdir, 2, 23);

  wattron(dlg_win_mkdir, attr_style);

  for (i = 23; i < max_x - 2; i++) {
    mvwaddch(dlg_win_mkdir, 2, i, ' ');
    mvwaddch(dlg_win_mkdir, 4, i, ' ');
  }
  mvwprintw(dlg_win_mkdir, 2, 24, "%s", curDirName);

  echo();  // 사용자의 입력을 화면에 표시하기 위함.
  keypad(dlg_win_mkdir, FALSE);
  mvwgetnstr(dlg_win_mkdir, 4, 24, usrIptName, max_x - 28);
  keypad(dlg_win_mkdir, TRUE);
  noecho();

  wattroff(dlg_win_mkdir, attr_style);

  mvwchgat(dlg_win_mkdir, 6, 24, 9, A_BOLD | A_UNDERLINE, 8, NULL);
  mvwchgat(dlg_win_mkdir, 6, 40, 9, A_NORMAL, 8, NULL);
  wmove(dlg_win_mkdir, 6, 24);

  while ((ipt = wgetch(dlg_win_mkdir)) != '\n') {
    switch (ipt) {
      case KEY_LEFT:
        wmove(dlg_win_mkdir, 6, 24);
        wchgat(dlg_win_mkdir, 9, A_BOLD | A_UNDERLINE, 8, NULL);
        mvwchgat(dlg_win_mkdir, 6, 40, 9, A_NORMAL, 8, NULL);
        wmove(dlg_win_mkdir, 6, 24);
        break;

      case KEY_RIGHT:
        wmove(dlg_win_mkdir, 6, 40);
        wchgat(dlg_win_mkdir, 9, A_BOLD | A_UNDERLINE, 8, NULL);
        mvwchgat(dlg_win_mkdir, 6, 24, 9, A_NORMAL, 8, NULL);
        wmove(dlg_win_mkdir, 6, 40);
        break;

      case KEY_UP:
        mvwchgat(dlg_win_mkdir, 6, 24, 9, A_NORMAL, 8, NULL);
        mvwchgat(dlg_win_mkdir, 6, 40, 9, A_NORMAL, 8, NULL);

        wattron(dlg_win_mkdir, attr_style);
        for (i = 23; i < max_x - 2; i++) {
          mvwaddch(dlg_win_mkdir, 4, i, ' ');
        }
        echo();  // 사용자의 입력을 화면에 표시하기 위함.
        keypad(dlg_win_mkdir, FALSE);
        mvwgetnstr(dlg_win_mkdir, 4, 24, usrIptName, max_x - 28);
        keypad(dlg_win_mkdir, TRUE);

        noecho();
        wattroff(dlg_win_mkdir, attr_style);
        wmove(dlg_win_mkdir, 6, 24);
        wchgat(dlg_win_mkdir, 9, A_BOLD | A_UNDERLINE, 8, NULL);

        break;
    }
    wrefresh(dlg_win_mkdir);
  }

  x = getcurx(dlg_win_mkdir);  // 현재 커서의 X 좌표만 가져오기
  // 사용자가 선택한 좌표의 위치로 [확인]인지 [취소]인지 선택하기로함.
  if ((x > 39 && x < 50) || usrIptName[0] == '\0')  // 취소 상태.
  {
    usrIptName[0] = '\0';
  } else {
    // 확인 이므로 디렉토리 생성 코드 실행.

    if (lstat(curDirName, &statbuf) == -1) {
      perror(" ");
      perm = 0000700;
    }
    perm =
        statbuf.st_mode;  // 바로 상위디렉토리의 권한을 그대로 이어받도록 하자.

    mkdir(usrIptName, perm);  // 디렉토리 생성.
  }

}  // Dlg_Mkdir() end

void Dlg_Rename(WINDOW *dlg_win_rename, const char *prevName, char *newName) {
  int y, x, max_x;  // max_y 제거 (사용하지 않으므로)
  int i;
  int attr_style;
  chtype ipt;

  newName[0] = '\0';
  keypad(dlg_win_rename, TRUE);  // initscr()이전의 이 함수 호출은 의미없음.

  max_x = getmaxx(dlg_win_rename);  // X 좌표만 가져오기

  box(dlg_win_rename, 0, 0);

  wattron(dlg_win_rename, A_BOLD);
  mvwprintw(dlg_win_rename, 0, 5, " 이름 변경 (Rename)");
  wattroff(dlg_win_rename, A_BOLD);

  attr_style = A_BOLD | COLOR_PAIR(8);
  wattron(dlg_win_rename, attr_style);
  mvwprintw(dlg_win_rename, 2, 3, " 이전 이름   ");

  mvwprintw(dlg_win_rename, 4, 3, " 새로운 이름 ");

  mvwprintw(dlg_win_rename, 6, 24, "  확 인  ");  // 버튼 1

  mvwprintw(dlg_win_rename, 6, 40, "  취 소  ");  // 버튼 2
  wattroff(dlg_win_rename, attr_style);

  attr_style = A_BOLD | COLOR_PAIR(7);
  wmove(dlg_win_rename, 2, 16);

  wattron(dlg_win_rename, attr_style);

  for (i = 16; i < max_x - 2; i++) {
    mvwaddch(dlg_win_rename, 2, i, ' ');
    mvwaddch(dlg_win_rename, 4, i, ' ');
  }
  mvwprintw(dlg_win_rename, 2, 17, "%s", prevName);

  echo();  // 사용자의 입력을 화면에 표시하기 위함.

  // mvwgetnstr(dlg_win_rename, 4, 17, newName, max_x-28);

  // mvwgetnstr으로는 디렉토리 생성이 오류가 나는 문자를 생성하게 되므로
  // 제외 문자를 둬보고 싶어서 백스페이스키와 영문 숫자 기타 특수문자만 허용하게
  // 해봄. 한글 이름 입력이 안 되는 문제가 있음.
  ipt = 0, i = 0;
  wmove(dlg_win_rename, 4, 17);
  getyx(dlg_win_rename, y, x);
  while ((ipt = wgetch(dlg_win_rename)) != '\n' && i < max_x - 28) {
    if (ipt == KEY_BACKSPACE) {
      if (i > 0) {
        wmove(dlg_win_rename, y, --x);
        waddch(dlg_win_rename, ' ');
        newName[i--] = '\0';
      }

      wmove(dlg_win_rename, y, x);
      continue;
    }

    if (ipt < 33 || ipt > 126)  // 무시해야할 문자.
    {
      wmove(dlg_win_rename, y, x);
      continue;
    }

    newName[i++] = ipt;
    getyx(dlg_win_rename, y, x);
  }
  newName[i] = '\0';
  //--------------------------------------------------------------

  noecho();

  wattroff(dlg_win_rename, attr_style);

  mvwchgat(dlg_win_rename, 6, 24, 9, A_BOLD | A_UNDERLINE, 8, NULL);
  mvwchgat(dlg_win_rename, 6, 40, 9, A_NORMAL, 8, NULL);

  wmove(dlg_win_rename, 6, 24);

  ipt = 0;
  while ((ipt = wgetch(dlg_win_rename)) != '\n') {
    switch (ipt) {
      case KEY_LEFT:
        wmove(dlg_win_rename, 6, 24);
        wchgat(dlg_win_rename, 9, A_BOLD | A_UNDERLINE, 8, NULL);
        mvwchgat(dlg_win_rename, 6, 40, 9, A_NORMAL, 8, NULL);
        wmove(dlg_win_rename, 6, 24);
        break;

      case KEY_RIGHT:
        wmove(dlg_win_rename, 6, 40);
        wchgat(dlg_win_rename, 9, A_BOLD | A_UNDERLINE, 8, NULL);
        mvwchgat(dlg_win_rename, 6, 24, 9, A_NORMAL, 8, NULL);
        wmove(dlg_win_rename, 6, 40);

        break;
      case KEY_UP:  // 확인 버튼에서 다시 방향키(상)을 눌러
        // 재입력을 받고 싶을 때를 위한 코드.
        mvwchgat(dlg_win_rename, 6, 24, 9, A_NORMAL, 8, NULL);
        mvwchgat(dlg_win_rename, 6, 40, 9, A_NORMAL, 8, NULL);

        wattron(dlg_win_rename, attr_style);
        for (i = 16; i < max_x - 2; i++) {
          mvwaddch(dlg_win_rename, 4, i, ' ');
        }
        echo();  // 사용자의 입력을 화면에 표시하기 위함.

        keypad(dlg_win_rename, FALSE);
        mvwgetnstr(dlg_win_rename, 4, 17, newName, max_x - 28);
        keypad(dlg_win_rename, FALSE);

        noecho();
        wattroff(dlg_win_rename, attr_style);
        wmove(dlg_win_rename, 6, 24);
        wchgat(dlg_win_rename, 9, A_BOLD | A_UNDERLINE, 8, NULL);
    }

    wrefresh(dlg_win_rename);
  }

  getyx(dlg_win_rename, y, x);  // 현재 커서의 좌표값얻어옴.

  if ((x > 39 && x < 50) || newName[0] == '\0')  // 취소 상태.
  {
    newName[0] = '\0';
  } else {
    // 확인 이므로 이름 변경 코드 실행.
    rename(prevName, newName);
  }

}  // Dlg_Rename() end

void Dlg_Chmod(WINDOW *dlg_win_chmod, int selectItemCount[],
               CLIP_ENTITY **clip_items) {
  int y, x, pre_y, pre_x, max_x;  // max_y 제거 (사용하지 않으므로)
  int i;
  mode_t perm = 0000000;
  int attr_style;
  chtype ipt;
  char modeform[] = "읽기[ ] / 쓰기[ ] / 실행[ ]";
  char mark_ch;
  struct stat statbuf;

  ipt = 0;

  if (*clip_items == (CLIP_ENTITY *)NULL || selectItemCount[1] == 0) {
    return;  // SelectedItemClipBoard()가 제대로 실행되지 않았다면
             // 함수 바로 종료.
  }

  keypad(dlg_win_chmod, TRUE);  // initscr()이전의 이 함수 호출은 의미없음.

  max_x = getmaxx(dlg_win_chmod);  // X 좌표만 가져오기

  box(dlg_win_chmod, 0, 0);

  wattron(dlg_win_chmod, A_BOLD);
  mvwprintw(dlg_win_chmod, 0, 5, " 귄한 변경 (Chmod)");
  wattroff(dlg_win_chmod, A_BOLD);

  wattron(dlg_win_chmod, A_BOLD | COLOR_PAIR(8));
  mvwprintw(dlg_win_chmod, 2, 3, " 선택된 개체 목록 ");

  mvwprintw(dlg_win_chmod, 4, 3, "  소유자    권한  ");
  mvwprintw(dlg_win_chmod, 5, 3, "  같은 그룹 권한  ");
  mvwprintw(dlg_win_chmod, 6, 3, "  다른 사람 권한  ");

  mvwprintw(dlg_win_chmod, 8, 24, "  확 인  ");  // 버튼 1

  mvwprintw(dlg_win_chmod, 8, 40, "  취 소  ");  // 버튼 2
  wattroff(dlg_win_chmod, A_BOLD | COLOR_PAIR(8));

  attr_style = A_BOLD | COLOR_PAIR(7);
  wmove(dlg_win_chmod, 4, 21);

  wattron(dlg_win_chmod, attr_style);

  for (i = 21; i < max_x - 2; i++) {
    mvwaddch(dlg_win_chmod, 2, i, ' ');
    mvwaddch(dlg_win_chmod, 4, i, ' ');
    mvwaddch(dlg_win_chmod, 5, i, ' ');
    mvwaddch(dlg_win_chmod, 6, i, ' ');
  }

  mvwprintw(dlg_win_chmod, 2, 22, "%s 포함 %d개 개체", (*clip_items)[0].name,
            selectItemCount[1]);
  // 클립보드 clip_items의 0 이름과 갯수 출력.
  mvwprintw(dlg_win_chmod, 4, 22, "%s", modeform);
  mvwprintw(dlg_win_chmod, 5, 22, "%s", modeform);
  mvwprintw(dlg_win_chmod, 6, 22, "%s", modeform);

  wattroff(dlg_win_chmod, attr_style);

CHMOD_SELECT:  // 사용자가 방향키(상)을 눌렀을 때 오는 goto위치
  wattron(dlg_win_chmod, attr_style);

  wmove(dlg_win_chmod, 4, 27);
  getyx(dlg_win_chmod, y, x);

  while ((ipt = wgetch(dlg_win_chmod)) != '\n') {
    switch (ipt) {
      case KEY_UP:
        if (y > 4) {
          wmove(dlg_win_chmod, --y, x);
        }
        break;

      case KEY_DOWN:
        if (y < 6) {
          wmove(dlg_win_chmod, ++y, x);
        }
        break;

      case KEY_RIGHT:
        if (x < 47) {
          wmove(dlg_win_chmod, y, x = x + 10);
        }
        break;

      case KEY_LEFT:
        if (x > 27) {
          wmove(dlg_win_chmod, y, x = x - 10);
        }
        break;

      case ' ':
        getyx(dlg_win_chmod, y, x);
        if (y == 4) {
          if (x == 27) {
            if (perm & 0400) {
              perm = perm & 0377;
              mark_ch = ' ';
            } else {
              perm = perm | 0400;
              mark_ch = '*';
            }
          } else if (x == 37) {
            if (perm & 0200) {
              perm = perm & 0577;
              mark_ch = ' ';
            } else {
              perm = perm | 0200;
              mark_ch = '*';
            }
          } else if (x == 47) {
            if (perm & 0100) {
              perm = perm & 0677;
              mark_ch = ' ';
            } else {
              perm = perm | 0100;
              mark_ch = '*';
            }
          }
        } else if (y == 5) {
          if (x == 27) {
            if (perm & 0040) {
              perm = perm & 0737;
              mark_ch = ' ';
            } else {
              perm = perm | 0040;
              mark_ch = '*';
            }
          } else if (x == 37) {
            if (perm & 0020) {
              perm = perm & 0757;
              mark_ch = ' ';
            } else {
              perm = perm | 0020;
              mark_ch = '*';
            }
          } else if (x == 47) {
            if (perm & 0010) {
              perm = perm & 0767;
              mark_ch = ' ';
            } else {
              perm = perm | 0010;
              mark_ch = '*';
            }
          }
        } else if (y == 6) {
          if (x == 27) {
            if (perm & 0004) {
              perm = perm & 0773;
              mark_ch = ' ';
            } else {
              perm = perm | 0004;
              mark_ch = '*';
            }
          } else if (x == 37) {
            if (perm & 0002) {
              perm = perm & 0774;
              mark_ch = ' ';
            } else {
              perm = perm | 0002;
              mark_ch = '*';
            }
          } else if (x == 47) {
            if (perm & 0001) {
              perm = perm & 0776;
              mark_ch = ' ';
            } else {
              perm = perm | 0001;
              mark_ch = '*';
            }
          }
        }

        pre_y = y, pre_x = x;
        mvwaddch(dlg_win_chmod, y, x, mark_ch);
        y = pre_y, x = pre_x;
        wmove(dlg_win_chmod, y, x);
        break;

    }  // switch 끝
  }

  // 변해야될  좌표 위치
  // owner   r(4,27) w(4,37) x(4,47)
  // group   r(5,27) w(5,37) x(5,47)
  // others  r(6,27) w(6,37) x(6,47)

  wattroff(dlg_win_chmod, attr_style);

  mvwchgat(dlg_win_chmod, 8, 24, 9, A_BOLD | A_UNDERLINE, 8, NULL);
  mvwchgat(dlg_win_chmod, 8, 40, 9, A_NORMAL, 8, NULL);

  wmove(dlg_win_chmod, 8, 24);

  while ((ipt = wgetch(dlg_win_chmod)) != '\n') {
    switch (ipt) {
      case KEY_LEFT:
        wmove(dlg_win_chmod, 8, 24);
        wchgat(dlg_win_chmod, 9, A_BOLD | A_UNDERLINE, 8, NULL);
        mvwchgat(dlg_win_chmod, 8, 40, 9, A_NORMAL, 8, NULL);
        wmove(dlg_win_chmod, 8, 24);
        break;

      case KEY_RIGHT:
        wmove(dlg_win_chmod, 8, 40);
        wchgat(dlg_win_chmod, 9, A_BOLD | A_UNDERLINE, 8, NULL);
        mvwchgat(dlg_win_chmod, 8, 24, 9, A_NORMAL, 8, NULL);
        wmove(dlg_win_chmod, 8, 40);
        break;
        // chgat류 함수에선 모양과 색상을 따로 분리해서 설정해야한다.

      case KEY_UP:
        mvwchgat(dlg_win_chmod, 8, 24, 9, A_NORMAL, 8, NULL);
        mvwchgat(dlg_win_chmod, 8, 40, 9, A_NORMAL, 8, NULL);

        goto CHMOD_SELECT;  // 사용자가 방향키(상)버튼 눌렀을 때 변경가능하도록
        // goto사용하는 것이 나을 것 같음.
        break;
    }

    wrefresh(dlg_win_chmod);
  }

  getyx(dlg_win_chmod, y, x);  // 현재 커서의 좌표값얻어옴.

  if (x > 7 && x < 25 && y == 8)  // 확인 상태.
  {
    // int chmod(const char *path, mode_t mode);

    for (i = 0; i < selectItemCount[1]; i++) {
      if (lstat((*clip_items)[i].name, &statbuf) == -1) {
        perror(" ");
      }

      perm = (statbuf.st_mode & 0777000) | perm;

      chmod((*clip_items)[i].name, perm);
    }
  }

  // 확인이든 취소든 동적 메모리 해제 필요.
  for (i = 0; i < selectItemCount[1]; i++) {
    free((char *)((*clip_items)[i].name));
    (*clip_items)[i].name = (char *)NULL;
  }

  free((char *)((*clip_items)[0].abspath));  // 절대경로 정보를 갖고있지만 이
                                             // 함수에선 사용안함.
  (*clip_items)[0].abspath = (char *)NULL;

  free((CLIP_ENTITY *)(*clip_items));
  (*clip_items) = (CLIP_ENTITY *)NULL;

  selectItemCount[0] = selectItemCount[1] = 0;

}  // Dlg_Chmod() end