## FilesPanel



- [x] Двойной клик на определённых файлах должен вызывать их открытие по URI (аналог действия Open File)
- [x] В filesTable и stagedFilesTable в столбце "Статус" выводить не букву, а иконку, сделав размер колонки фиксированным
- [x] Разделить колонки на Name и Relative Directory
- [x] Обновлять панели после изменений файлов на диске или после выполнения git-действий
- [x] В контекстном меню для модифицированных файлов отображать название пункта "добавить" как "Фиксировать", а для недобавленных как "Добавить"
- [ ] Название директории сделать серым и без слеща в конце
- [ ] Добавить панель с директориями (как фильтр)
- [x] Добавить в контекстное меню пункты:
  - [ ] Revert
  - [x] Copy file path
  - [x] Open file
  - [x] Open folder
  - [x] Delete
  - [x] Blame
  - [x] Discard

## DiffEditor

- [x] Заменить шрифт на Consolas
- [ ] Для графических файлов отображать текущую картинку
- [ ] Позволять делать stage и revert для отдельных строк кода
- [x] Поправить нумерацию строк
- [ ] Разделить название файла и путь к директории
- [ ] Index Editor
  - [ ] 



## Repositories

- [ ] Группы репозитариев
- [ ] Favorite



## Menu

- [ ] Добавить в главное меню:

  - [ ] File

  - [ ] Edit

    - [ ] gitignore

  - [ ] View

    - [ ] Repositories
    - [ ] Journal
    - [ ] Log
    - [ ] Branches
    - [ ] Commit message
    - [ ] Diff
    - [ ] Changes
    - [ ] Directory

  - [ ] Local

    - [ ] Create Branch

  - [ ] Remote

    - [ ] Add remote

  - [ ] Help

    - [ ] Reference
    - [ ] About

    



## Hotkeys

- [ ] Ctrl+t - застейджить файл
- [ ] Ctrl+shift+t - отстэйджить
- [x] Ctrl+Enter - коммит
- [ ] Esc - убрать выделение файла (очистить DiffEditor)
- [ ] F6 - следующее изменение
- [ ] shift+F6 - предыдущее изменение
- [ ] Push
- [ ] Pull



## Settings

- [x] Добавить выбор и размер шрифта для DiffEditor



## BranchesWidget

- [x] Добавить виджет branches
- [x] Отображать все ветки
- [x] Переключаться между ветками
- [x] Создавать ветку
- [x] Удалять ветку
- [x] Переименовывать ветку
- [x] Добавлять remote
- [ ] Добавлять Tags
- [x] Добавлять Stashes
- [ ] Создание Merge



## Из дополнительного

- [ ] The **Reflog** view shows the chronological history of `HEAD` to help you access “lost” commits.
- [ ] **Submodules**
- [ ] GraphView
- [ ] Blame
- [ ] Bisect
- [ ] Conflict solver
- [ ] Save Patch
- [ ] Apply Patch