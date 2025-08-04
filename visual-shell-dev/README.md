# 개선 진행

> 몇십년 전에 졸업 작품으로 내고 이후 수정이 없던 코드를 이제 조금씩 수정해보자...
>
> `C`와 `Unix` 시스템 콜 함수들 기억이 안나서 잘 될지는 모르겠지만... 😅

### 개발 환경

* OS환경: `Ubuntu WSL` 또는 가상머신의 `Rocky Linux`
* 개발도구: `VSCode + WSL/SSH Remote`, `GCC`

### 라이브러리 설치
#### Ubuntu WSL
```sh
sudo apt install libncurses-dev
```

#### CentOS / Rocky Linux
```sh
sudo dnf install ncurses-devel
```