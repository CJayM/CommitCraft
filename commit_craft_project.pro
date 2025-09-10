TEMPLATE = subdirs

SUBDIRS += \
    commit_craft_app \
    commit_craft_lib \
    commit_craft_tests

commit_craft_app.depends = commit_craft_lib
commit_craft_tests.depends = commit_craft_lib
