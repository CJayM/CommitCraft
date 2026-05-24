## FilesPanel



- [x] Двойной клик на определённых файлах должен вызывать их открытие по URI (аналог действия Open File)
- [x] В filesTable и stagedFilesTable в столбце "Статус" выводить не букву, а иконку, сделав размер колонки фиксированным
- [x] Разделить колонки на Name и Relative Directory
- [x] Обновлять панели после изменений файлов на диске или после выполнения git-действий
- [x] В контекстном меню для модифицированных файлов отображать название пункта "добавить" как "Фиксировать", а для недобавленных как "Добавить"
- [ ] Название директории сделать серым и без слеша в конце
- [x] Добавить панель с директориями (как фильтр)
- [x] Добавить в контекстное меню пункты:
  - [x] Copy file path
  - [x] Open file
  - [x] Open folder
  - [x] Delete
  - [x] Blame
  - [x] Discard

## DiffEditor

- [x] Заменить шрифт на Consolas
- [x] Для графических файлов отображать текущую картинку
- [x] Позволять делать stage и revert для отдельных строк кода
- [x] Поправить нумерацию строк
- [ ] Разделить название файла и путь к директории
- [ ] Index Editor
  - [ ] 



## Branches

- [ ] Добавить удаление для веток у Remotes
- [ ] Убрать Apply/Drop у не Stash



## Repositories

- [ ] Группы репозитариев
- [ ] Favorite



## Menu

- [ ] Добавить в главное меню:

  - [ ] File

  - [x] Edit

    - [x] gitignore

  - [ ] View

    - [ ] Repositories
    - [x] Journal
    - [x] Log
    - [x] Branches
    - [ ] Commit message
    - [x] Diff
    - [x] Changes
    - [x] Directory

  - [ ] Local

    - [ ] Create Branch

  - [ ] Remote

    - [ ] Add remote

  - [ ] Help

    - [ ] Reference
    - [ ] About

    



## Hotkeys

- [x] Ctrl+t - застейджить файл
- [x] Ctrl+shift+t - отстэйджить
- [x] Ctrl+Enter - коммит
- [x] Esc - убрать выделение файла (очистить DiffEditor)
- [x] F6 - следующее изменение
- [x] shift+F6 - предыдущее изменение
- [x] F7 - Push
- [x] F8 - Pull



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
- [x] GraphView
- [ ] Blame
- [ ] Bisect
- [ ] Conflict solver
- [ ] Save Patch
- [ ] Apply Patch