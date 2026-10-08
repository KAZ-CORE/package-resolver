#include "headers/cache.h"
#include "headers/argument.h"
#include <solv/pool.h>
#include <solv/repo.h>

const char *ARCHS[] = {
    "amd64",
    "i386",
    "arm64",
    "armhf",
    "armel",
    "mips64el",
    "ppc64el",
    "riscv64",
    "s390x"
};

static inline bool check_arch(const char* arch)
{
    for (int i = 0; i < SIZE_ARRAY_ARCHS; i++)
    {
        if(strcmp(ARCHS[i], arch) == 0)
            return true;
    }

    fprintf(stderr,
        "Invalid architecture %s\n",arch);
    return false;
}


static inline bool check_file(const char *path)
{
    struct stat st;

    if (stat(path, &st) != 0)
    {
        fprintf(stderr,
            "File not found: %s: ", path);
        perror("");
        return false;
    }

    if (!S_ISREG(st.st_mode))
    {
        fprintf(stderr,
            "Not a regular file: %s\n", path);
        return false;
    }

    return true;
}

static inline delimiter_positions get_delimiter_positions (
    const char* str,
    const char delimiter
)
{
    char *addr_dot1 = strchr(str, delimiter);
    if (!addr_dot1)
        return (delimiter_positions){ -1, -1 };

    char *addr_dot2 = strchr(addr_dot1 + 1, delimiter);
    if (!addr_dot2)
        return (delimiter_positions){ -1, -1 };

    return (delimiter_positions){addr_dot1 - str, addr_dot2 - str};
}

static inline void get_arch(
    char *buffer,
    int size_buffer,
    delimiter_positions pos_dots,
    const char *str
)
{

    if (pos_dots.pos1 == -1 && pos_dots.pos2 == -1)
        return;

    int size_target = pos_dots.pos2 - pos_dots.pos1 - 1;

    if (size_buffer < size_target + 1) {
        buffer[0] = '\0';
        return;
    }
    strncpy(
        buffer,
        str + pos_dots.pos1 + 1,
        size_target
    );

    buffer[size_target] = '\0';
}

static inline bool load_debpackages (
    Pool **pool,
    Repo **repo,
    const char* name_file,
    const char* name_repo,
    const char* arch
)
{
    FILE *fp;

    *pool = pool_create();

    if (!*pool) {
        fprintf(stderr,
            "can not create Pool!\n");
        return false;
    }
    pool_setarch(*pool, arch);

    *repo = repo_create(*pool, name_repo);

    if (!*repo)
    {
        fprintf(stderr,
            "can not create Repo\n");
        pool_free(*pool);
        *pool = NULL;
        return false;
    }

    fp = fopen(name_file, "r");

    if (!fp)
    {
        perror(name_file);
        repo_free(*repo, 0);
        pool_free(*pool);
        *pool = NULL;
        *repo = NULL;
        return false;
    }

    if (repo_add_debpackages(*repo, fp, 0))
    {
        fprintf(stderr,
            "Failed to parse Packages\n");
        fclose(fp);
        repo_free(*repo, 0);
        pool_free(*pool);
        *pool = NULL;
        *repo = NULL;
        return false;
    }

    fclose(fp);

    if ((*repo)->nsolvables <= 0)
    {
        fprintf(stderr,
            "No packages found in Packages file: %s\n", name_file);
        repo_free(*repo, 0);
        pool_free(*pool);
        *pool = NULL;
        *repo = NULL;
        return false;
    }

    return true;
}

static inline bool load_solvpackages (
    Pool **pool,
    Repo **repo,
    const char* name_file
)
{

    FILE *fp;

    *pool = pool_create();

    if (!*pool) {
        fprintf(stderr,
            "can not create Pool!\n");
        return false;
    }

    if (pool_setdisttype(*pool, DISTTYPE_DEB) < 0)
    {
        fprintf(stderr,
                "can not set Pool disttype to Debian!\n");

        pool_free(*pool);
        *pool = NULL;

        return false;
    }

    *repo = repo_create(*pool, "debian");

    if (!*repo)
    {
        fprintf(stderr,
            "can not create Repo!\n");
        pool_free(*pool);
        *pool = NULL;
        *repo = NULL;
        return false;
    }

    delimiter_positions pos_dots = get_delimiter_positions(
        name_file,
        '.'
    );

    if(pos_dots.pos1 == -1 && pos_dots.pos2 == -1) {
        fprintf(stderr,
            "%s file name is invalid.\n",name_file);
        repo_free(*repo, 0);
        pool_free(*pool);
        *pool = NULL;
        *repo = NULL;
        return false;
    }

    int size_arch = (pos_dots.pos2 - pos_dots.pos1 - 1) + 1;
    char arch[size_arch];

    get_arch(
        arch,
        size_arch,
        pos_dots,
        name_file
    );

    if(!check_arch(arch)) {
        repo_free(*repo, 0);
        pool_free(*pool);
        *pool = NULL;
        *repo = NULL;
        return false;
    }

    pool_setarch(*pool, arch);

    fp = fopen(name_file, "rb");

    if (!fp)
    {
        perror(name_file);
        repo_free(*repo,0);
        pool_free(*pool);
        *repo = NULL;
        *pool = NULL;
        return false;
    }

    if (repo_add_solv(*repo, fp, 0) != 0)
    {
        fprintf(stderr,
                "Failed to load %s: %s\n",
                name_file,
                pool_errstr(*pool));

        fclose(fp);
        repo_free(*repo,0);
        pool_free(*pool);
        *repo = NULL;
        *pool = NULL;
        return false;
    }

    fclose(fp);
    return true;
}

static inline bool get_name_filename (
    char *filename,
    char *buffer,
    size_t size_buffer
)
{


    size_t size_filename = strlen(filename);
    size_t size_name = 0;
    bool found = false;

    if (size_filename >= size_buffer)
        return false;

    for (int i = 0; i < size_filename; i++) {
        if (filename[i] == '_') {
            size_name = i;
            found = true;
            break;
        }
        buffer[i] = filename[i];
    }

    if (!found)
        return false;

    buffer[size_name] = '\0';

    return true;

}

static inline bool get_filename_location(
    char *location,
    char *buffer,
    size_t size_buffer
)
{
    size_t size_location = strlen(location);
    int pos = -1;

    if (size_location >= size_buffer)
        return false;

    for (int i = size_location; i > 0; i--)
    {
        if (location[i-1] == '/') {
            pos = i-1;
            break;
        }
    }


    size_t size_cpy = size_location - pos - 1;


    if (pos == -1 || size_cpy == 0)
        return false;

    strncpy(buffer,&location[pos + 1], size_cpy);
    buffer[size_cpy] = '\0';

    return true;
}

static inline bool get_name_location (
    char *location,
    char *buffer,
    size_t size_buffer
)
{

    char filename[size_buffer];
    if(!get_filename_location(
        location,
        filename,
        size_buffer
    ))
        return false;

    if(!get_name_filename(
        filename,
        buffer,
        size_buffer
    ))
        return false;


    return true;

}

static inline bool get_name_location_or_filename(
    char *location_or_filename,
    char *buffer,
    size_t size_buffer
)
{

    if(get_name_location(
        location_or_filename,
        buffer,
        size_buffer
    ))
        return true;

    if(get_name_filename(
        location_or_filename,
        buffer,
        size_buffer
    ))
        return true;

    return false;
}

static void found_packages_free(found_packages found)
{
    free(found.found);
    free(found.notfound);
}

static inline found_packages find_packages(
    Packages packages,
    Pool **pool
)
{
    found_packages found = {0};

    found.found = malloc(
        packages.count * sizeof(*found.found)
    );

    found.notfound = malloc(
        packages.count * sizeof(*found.notfound)
    );

    if (!found.found || !found.notfound)
    {
        found_packages_free(found);

        found.found = NULL;
        found.notfound = NULL;
        found.error = true;

        return found;
    }

    pool_createwhatprovides(*pool);

    for (int i = 0; i < packages.count; i++)
    {
        char *name_package = packages.names[i];

        size_t size_buffer = strlen(packages.names[i]) + 1;
        char buffer[size_buffer];

        if (get_name_location_or_filename(
            packages.names[i],
            buffer,
            size_buffer
        ))
            name_package = buffer;

            Id package = pool_str2id(*pool, name_package, 0);

            if (package)
            {
                Id *p = pool_whatprovides_ptr(*pool, package);

                bool found_pkg = false;

                while (*p)
                {
                    Solvable *s = pool_id2solvable(*pool, *p);

                    if (s && s->name == package)
                    {
                        found.found[found.size_found] =
                        pool_id2str(*pool, s->name);

                        found.size_found++;

                        found_pkg = true;

                        break;
                    }

                    p++;
                }

                if (!found_pkg)
                {
                    found.notfound[found.size_notfound] =
                    packages.names[i];

                    found.size_notfound++;
                }
            }
            else
            {
                found.notfound[found.size_notfound] =
                packages.names[i];

                found.size_notfound++;
            }
    }

    return found;
}


static inline void get_filename_free (filename fname)
{
    free(fname.filenames);
    free(fname.locations);
    free(fname.notfound_pkgs);
}

static inline filename get_filename (
    Packages packages,
    Pool **pool
)
{

    filename fname = {0};

    fname.filenames = malloc(
        packages.count * sizeof(*fname.filenames)
    );

    fname.locations = malloc(
        packages.count * sizeof(*fname.locations)
    );

    fname.notfound_pkgs = malloc(
        packages.count * sizeof(*fname.notfound_pkgs)
    );

    if (!fname.filenames ||
        !fname.locations ||
        !fname.notfound_pkgs)
    {
        fname.error = true;
        return fname;
    }

    found_packages found = find_packages(packages,pool);

    if (found.error)
    {
        fname.error = true;
        found_packages_free(found);
        return fname;
    }

    memcpy(
        fname.notfound_pkgs,
        found.notfound,
        sizeof(*found.notfound)*found.size_notfound
    );
    fname.size_notfound_pkgs =
        found.size_notfound;

    pool_createwhatprovides(*pool);

    for (int i = 0; i < found.size_found; i++)
    {

        Id pkg = pool_str2id(*pool, found.found[i], 0);

        if (!pkg)
        {
            fname.notfound_pkgs[fname.size_notfound_pkgs] =
                found.found[i];
            fname.size_notfound_pkgs++;
            continue;
        }

        Id *p = pool_whatprovides_ptr(*pool, pkg);

        bool check_found = false;

        while (*p)
        {
            Solvable *s = pool_id2solvable(*pool, *p);

            if (s && s->name == pkg)
            {
                unsigned int medianr = 0;

                const char *location =
                    solvable_lookup_location(s, &medianr);

                const char *filename =
                    solvable_lookup_str(s, SOLVABLE_MEDIAFILE);

                if (location && filename)
                {
                    check_found = true;

                    fname.filenames[fname.size_filenames] =
                        filename;
                    fname.size_filenames++;

                    fname.locations[fname.size_locations] =
                        location;
                    fname.size_locations++;

                }

                break;
            }

            p++;
        }

        if (!check_found)
        {
            fname.notfound_pkgs[fname.size_notfound_pkgs] =
                found.found[i];
            fname.size_notfound_pkgs++;
        }

    }

    found_packages_free(found);
    return fname;
}


static inline void found_depends_free(
    found_depends fmain_depends
)
{
    if (fmain_depends.found)
    {
        for (size_t i = 0; i < fmain_depends.size_size_found; i++)
        {
            free(fmain_depends.found[i]);
        }

        free(fmain_depends.found);
    }

    free(fmain_depends.size_found);
    free(fmain_depends.notfound);
}


static inline found_depends get_main_depends(
    Packages packages,
    Pool **pool
)
{

    found_depends fmain_depends = {0};

    fmain_depends.found = calloc(
        packages.count,
        sizeof(*fmain_depends.found)
    );

    fmain_depends.size_found = calloc(
        packages.count,
        sizeof(*fmain_depends.size_found)
    );

    fmain_depends.size_size_found = packages.count;

    fmain_depends.notfound = calloc(
        packages.count,
        sizeof(*fmain_depends.notfound)
    );


    if (!fmain_depends.notfound ||
        !fmain_depends.found ||
        !fmain_depends.size_found
    )
    {
        fmain_depends.error = true;
        return fmain_depends;
    }

    found_packages found = find_packages(packages,pool);

    if (found.error)
    {
        fmain_depends.error = true;
        found_packages_free(found);
        return fmain_depends;
    }

    memcpy(
        fmain_depends.notfound,
        found.notfound,
        sizeof(*found.notfound)*found.size_notfound
    );

    fmain_depends.size_notfound =
        found.size_notfound;

    for (size_t i = 0; i < found.size_found; i++)
    {

        Id pkg = pool_str2id(*pool, found.found[i], 0);

        Id *p = pool_whatprovides_ptr(*pool, pkg);

        bool found_pkg = false;

        while (*p)
        {
            Solvable *s = pool_id2solvable(*pool, *p);

            if (s && s->name == pkg)
            {

                found_pkg = true;

                Queue deps;
                queue_init(&deps);

                if (solvable_lookup_deparray(
                        s,
                        SOLVABLE_REQUIRES,
                        &deps,
                        0))
                {

                    const char **depends = malloc(
                        deps.count * sizeof(*depends)
                    );

                    if (!depends)
                    {
                        fmain_depends.error = true;
                        queue_free(&deps);
                        found_packages_free(found);
                        return fmain_depends;
                    }

                    fmain_depends.found[i] = depends;

                    for (int j = 0; j < deps.count; j++)
                    {
                        fmain_depends.found[i][j] =
                            pool_id2str(*pool, deps.elements[j]);

                        fmain_depends.size_found[i]++;
                    }

                }

                queue_free(&deps);

                break;
            }

            p++;
        }

    }

    found_packages_free(found);
    return fmain_depends;

}


static inline found_depends get_all_depends(
    Packages packages,
    Pool **pool
)
{
    found_depends fall_depends = {0};

    fall_depends.found = calloc(
        packages.count,
        sizeof(*fall_depends.found)
    );

    fall_depends.size_found = calloc(
        packages.count,
        sizeof(*fall_depends.size_found)
    );

    fall_depends.size_size_found =
    packages.count;

    fall_depends.notfound = calloc(
        packages.count,
        sizeof(*fall_depends.notfound)
    );

    if (!fall_depends.found ||
        !fall_depends.size_found ||
        !fall_depends.notfound)
    {
        fall_depends.error = true;
        return fall_depends;
    }

    found_packages found =
    find_packages(
        packages,
        pool
    );

    if (found.error)
    {
        fall_depends.error = true;

        found_packages_free(found);

        return fall_depends;
    }

    memcpy(
        fall_depends.notfound,
        found.notfound,
        sizeof(*found.notfound) *
        found.size_notfound
    );

    fall_depends.size_notfound =
    found.size_notfound;

    pool_createwhatprovides(*pool);

    for (size_t i = 0;
         i < found.size_found;
    i++)
         {
             Id pkg =
             pool_str2id(
                 *pool,
                 found.found[i],
                 0
             );

             if (!pkg)
             {
                 fall_depends.notfound[
                     fall_depends.size_notfound++
                 ] = found.found[i];

                 continue;
             }

             Id root_id = 0;

             Id *providers =
             pool_whatprovides_ptr(
                 *pool,
                 pkg
             );

             while (*providers)
             {
                 Id id = *providers;

                 if (id > 1)
                 {
                     Solvable *s =
                     pool_id2solvable(
                         *pool,
                         id
                     );

                     if (s && s->name == pkg)
                     {
                         root_id = id;
                         break;
                     }
                 }

                 providers++;
             }

             if (!root_id)
             {
                 fall_depends.notfound[
                     fall_depends.size_notfound++
                 ] = found.found[i];

                 continue;
             }

             Solver *solver =
             solver_create(*pool);

             if (!solver)
             {
                 fall_depends.error = true;

                 found_packages_free(found);

                 return fall_depends;
             }

             solver_set_flag(
                 solver,
                 SOLVER_FLAG_IGNORE_RECOMMENDED,
                 1
             );

             Queue job;
             queue_init(&job);

             queue_push(
                 &job,
                 SOLVER_INSTALL |
                 SOLVER_SOLVABLE
             );

             queue_push(
                 &job,
                 root_id
             );

             if (solver_solve(
                 solver,
                 &job
             ) != 0)
             {
                 queue_free(&job);
                 solver_free(solver);

                 fall_depends.error = true;

                 found_packages_free(found);

                 return fall_depends;
             }

             Transaction *trans =
             solver_create_transaction(
                 solver
             );

             if (!trans)
             {
                 queue_free(&job);
                 solver_free(solver);

                 fall_depends.error = true;

                 found_packages_free(found);

                 return fall_depends;
             }

             Queue installed;
             queue_init(&installed);

             transaction_installedresult(
                 trans,
                 &installed
             );

             const char **depends = NULL;

             if (installed.count > 0)
             {
                 depends = malloc(
                     installed.count *
                     sizeof(*depends)
                 );

                 if (!depends)
                 {
                     queue_free(&installed);
                     transaction_free(trans);
                     queue_free(&job);
                     solver_free(solver);

                     fall_depends.error = true;

                     found_packages_free(found);

                     return fall_depends;
                 }
             }

             Queue seen;
             queue_init(&seen);

             size_t count = 0;

             for (int j = 0;
                  j < installed.count;
             j++)
                  {
                      Id id =
                      installed.elements[j];

                      if (id <= 1 ||
                          id == root_id)
                          continue;

                      bool duplicate = false;

                      for (int k = 0;
                           k < seen.count;
                      k++)
                           {
                               if (seen.elements[k] == id)
                               {
                                   duplicate = true;
                                   break;
                               }
                           }

                           if (duplicate)
                               continue;

                      Solvable *s =
                      pool_id2solvable(
                          *pool,
                          id
                      );

                      if (!s)
                          continue;

                      queue_push(
                          &seen,
                          id
                      );

                      depends[count++] =
                      pool_id2str(
                          *pool,
                          s->name
                      );
                  }

                  if (count > 0)
                  {
                      fall_depends.found[i] =
                      depends;

                      fall_depends.size_found[i] =
                      count;
                  }
                  else
                  {
                      free(depends);
                  }

                  queue_free(&seen);
                  queue_free(&installed);
                  transaction_free(trans);
                  queue_free(&job);
                  solver_free(solver);
         }

         found_packages_free(found);

         return fall_depends;
}

static int compare_strings(const void *a, const void *b)
{
    const char *const *sa = a;
    const char *const *sb = b;

    return strcmp(*sa, *sb);
}

static inline unique_depends get_unique_depends(
    found_depends deps
)
{
    unique_depends result = {0};

    size_t total = 0;

    for (size_t i = 0;
         i < deps.size_size_found;
    i++)
         {
             total += deps.size_found[i];
         }

         if (!total)
             return result;

     const char **all = malloc(
        total * sizeof(*all)
    );

    if (!all)
    {
        result.error = true;
        return result;
    }
    size_t pos = 0;

    for (size_t i = 0;
         i < deps.size_size_found;
    i++)
         {
             for (size_t j = 0;
                  j < deps.size_found[i];
             j++)
                  {
                      all[pos++] = deps.found[i][j];
                  }
         }

         qsort(
             all,
             total,
             sizeof(*all),
               compare_strings
         );

         size_t count = 0;

         for (size_t i = 0;
              i < total;
    i++)
              {
                  if (i &&
                      strcmp(all[i], all[i - 1]) == 0)
                  {
                      continue;
                  }

                  count++;
              }

              result.found = malloc(
                  count * sizeof(*result.found)
              );

              if (!result.found)
              {
                  free(all);
                  result.error = true;
                  return result;
              }

              result.size_found = count;

              size_t index = 0;

              for (size_t i = 0;
                   i < total;
    i++)
                   {
                       if (i &&
                           strcmp(all[i], all[i - 1]) == 0)
                       {
                           continue;
                       }

                       result.found[index++] = all[i];
                   }

                   free(all);

                   return result;
}

static inline void unique_depends_free(
    unique_depends depends
)
{
    free(depends.found);
}


bool create_cache (const char *name, const char *arch, const char* name_file_in)
{
    if(!check_arch(arch) || !check_file(name_file_in))
        return false;

    Pool *pool;
    Repo *repo;
    FILE *out;

    if(!load_debpackages(
        &pool,
        &repo,
        name_file_in,
        "debian",
        arch
    ))
        return false;

    char name_file_out[strlen(name)+strlen(arch)+7];
    sprintf(name_file_out,"%s.%s.solv",name,arch);
    out = fopen(name_file_out, "wb");

    if (!out)
    {
        perror(name_file_out);
        repo_free(repo, 0);
        pool_free(pool);
        return false;
    }

    if (repo_write(repo, out))
    {
        fprintf(stderr,
            "Failed to write packages.solv\n");
        fclose(out);
        repo_free(repo, 0);
        pool_free(pool);
        return false;
    }

    fclose(out);
    repo_free(repo, 0);
    pool_free(pool);

    fprintf(stdout,
        "Created %s\n",name_file_out);

    return true;
}

bool show_find (Packages find_pkgs, const char* name_file)
{

    Pool *pool;
    Repo *repo;

    if(!load_solvpackages(
        &pool,
        &repo,
        name_file
    ))
        return false;


    found_packages found = find_packages(find_pkgs, &pool);

    if (found.error)
    {
        fprintf(stderr,
                "Failed to find packages: an internal error occurred during package lookup.\n"
        );
        found_packages_free(found);
        repo_free(repo, 0);
        pool_free(pool);
        return false;
    }

    for (int i = 0; i < found.size_found; i++)
        fprintf(stdout,"%s ",found.found[i]);

    for (int i = 0; i < found.size_notfound; i++)
        fprintf(stderr,"[not_found:%s]",found.notfound[i]);
    printf("\n");

    found_packages_free(found);
    repo_free(repo, 0);
    pool_free(pool);
    return true;
}

bool show_filename (Packages packages, const char* name_file)
{

    Pool *pool;
    Repo *repo;

    if(!load_solvpackages(
        &pool,
        &repo,
        name_file
    ))
        return false;


    filename fname = get_filename(packages, &pool);

    if (fname.error)
    {
        get_filename_free(fname);
        repo_free(repo, 0);
        pool_free(pool);
        return false;
    }

    for(int i = 0; i < fname.size_locations; i++)
    {
        fprintf(
            stdout,
            " %s",
            fname.filenames[i]
        );
    }
    fprintf(
        stdout,
        "\n"
    );


    for(int i = 0; i < fname.size_notfound_pkgs; i++)
    {
        fprintf(
            stderr,
            " [%s]",
            fname.notfound_pkgs[i]
        );
    }
    fprintf(
        stderr,
        "\n"
    );


    get_filename_free(fname);
    repo_free(repo, 0);
    pool_free(pool);

    return true;
}

bool show_location (Packages packages, const char* name_file)
{
    Pool *pool;
    Repo *repo;

    if(!load_solvpackages(
        &pool,
        &repo,
        name_file
    ))
        return false;


    filename fname = get_filename(packages, &pool);

    if (fname.error)
    {
        get_filename_free(fname);
        repo_free(repo, 0);
        pool_free(pool);
        return false;
    }


    for(int i = 0; i < fname.size_locations; i++)
        fprintf(
            stdout,
            " %s",
            fname.locations[i]
        );

    fprintf(stdout,"\n");


    for(int i = 0; i < fname.size_notfound_pkgs; i++)
        fprintf(
            stderr,
            " [%s]",
            fname.notfound_pkgs[i]
        );

    fprintf(stderr,"\n");

    get_filename_free(fname);
    repo_free(repo, 0);
    pool_free(pool);
    return true;
}

bool show_main_depends (Packages packages, const char* name_file)
{

    Pool *pool;
    Repo *repo;

    if(!load_solvpackages(
        &pool,
        &repo,
        name_file
    ))
        return false;

    found_depends fmain_depends = get_main_depends(
        packages,
        &pool
    );

    if (fmain_depends.error)
    {
        found_depends_free(fmain_depends);
        repo_free(repo, 0);
        pool_free(pool);
        return false;
    }

    unique_depends unique =
        get_unique_depends(fmain_depends);

    if (unique.error)
    {
        found_depends_free(fmain_depends);
        repo_free(repo, 0);
        pool_free(pool);
        return false;
    }

    for (int i = 0; i < unique.size_found; i++)
    {
        fprintf(
            stdout,
            " %s",
            unique.found[i]
        );
    }
    fprintf(
        stdout,
        "\n"
    );

    for(int i = 0; i < fmain_depends.size_notfound; i++)
    {
        fprintf(
            stderr,
            " [%s]",
            fmain_depends.notfound[i]
        );
    }
    fprintf(
        stderr,
        "\n"
    );

    unique_depends_free(unique);
    found_depends_free(fmain_depends);
    repo_free(repo, 0);
    pool_free(pool);
    return true;
}


bool show_main_depends__show_filename (
    Packages packages,
    const char* name_file
)
{
    Pool *pool;
    Repo *repo;

    if(!load_solvpackages(
        &pool,
        &repo,
        name_file
    ))
        return false;

    found_depends fmain_depends = get_main_depends(
        packages,
        &pool
    );

    if (fmain_depends.error)
    {
        found_depends_free(fmain_depends);
        repo_free(repo,0);
        pool_free(pool);
        return false;
    }

    unique_depends unique =
        get_unique_depends(fmain_depends);


    if (unique.error)
    {
        found_depends_free(fmain_depends);
        repo_free(repo,0);
        pool_free(pool);
        return false;
    }

    filename fname = get_filename(
        (Packages){(char **)unique.found, unique.size_found},
        &pool
    );


    for(int i = 0; i < fname.size_locations; i++)
    {
        fprintf(
            stdout,
            " %s",
            fname.filenames[i]
        );
    }
    fprintf(
        stdout,
        "\n"
    );


    for(int i = 0; i < fname.size_notfound_pkgs; i++)
    {
        fprintf(
            stderr,
            " [%s]",
            fname.notfound_pkgs[i]
        );
    }
    fprintf(
        stderr,
        "\n"
    );


    get_filename_free(fname);
    unique_depends_free(unique);
    found_depends_free(fmain_depends);

    repo_free(repo, 0);
    pool_free(pool);

    return true;
}

bool show_main_depends__show_location (
    Packages packages,
    const char* name_file
)
{
    Pool *pool;
    Repo *repo;

    if(!load_solvpackages(
        &pool,
        &repo,
        name_file
    ))
        return false;

    found_depends fmain_depends = get_main_depends(
        packages,
        &pool
    );

    if (fmain_depends.error)
    {
        found_depends_free(fmain_depends);
        repo_free(repo,0);
        pool_free(pool);
        return false;
    }

    unique_depends unique =
        get_unique_depends(fmain_depends);


    if (unique.error)
    {
        found_depends_free(fmain_depends);
        repo_free(repo,0);
        pool_free(pool);
        return false;
    }

    filename fname = get_filename(
        (Packages){(char **)unique.found, unique.size_found},
        &pool
    );


    for(int i = 0; i < fname.size_locations; i++)
    {
        fprintf(
            stdout,
            " %s",
            fname.locations[i]
        );
    }
    fprintf(
        stdout,
        "\n"
    );


    for(int i = 0; i < fname.size_notfound_pkgs; i++)
    {
        fprintf(
            stderr,
            " [%s]",
            fname.notfound_pkgs[i]
        );
    }
    fprintf(
        stderr,
        "\n"
    );


    get_filename_free(fname);
    unique_depends_free(unique);
    found_depends_free(fmain_depends);

    repo_free(repo, 0);
    pool_free(pool);

    return true;
}

bool show_all_depends (Packages packages, const char* name_file)
{


    Pool *pool;
    Repo *repo;

    if(!load_solvpackages(
        &pool,
        &repo,
        name_file
    ))
        return false;

    found_depends fall_depends = get_all_depends(
        packages,
        &pool
    );

    if (fall_depends.error)
    {
        found_depends_free(fall_depends);
        repo_free(repo, 0);
        pool_free(pool);
        return false;
    }

    unique_depends unique =
        get_unique_depends(fall_depends);


    if (unique.error)
    {
        found_depends_free(fall_depends);
        repo_free(repo, 0);
        pool_free(pool);
        return false;
    }

    for (int i = 0; i < unique.size_found; i++)
    {
        fprintf(
            stdout,
            " %s",
            unique.found[i]
        );
    }
    fprintf(
        stdout,
        "\n"
    );

    for(int i = 0; i < fall_depends.size_notfound; i++)
    {
        fprintf(
            stderr,
            " [%s]",
            fall_depends.notfound[i]
        );
    }
    fprintf(
        stderr,
        "\n"
    );

    unique_depends_free(unique);
    found_depends_free(fall_depends);
    repo_free(repo, 0);
    pool_free(pool);
    return true;
}


bool show_all_depends__show_filename (
    Packages packages,
    const char* name_file
)
{
    Pool *pool;
    Repo *repo;

    if(!load_solvpackages(
        &pool,
        &repo,
        name_file
    ))
        return false;

    found_depends fall_depends = get_all_depends(
        packages,
        &pool
    );

    if (fall_depends.error)
    {
        found_depends_free(fall_depends);
        repo_free(repo,0);
        pool_free(pool);
        return false;
    }


    unique_depends unique =
        get_unique_depends(fall_depends);

    if (unique.error)
    {
        found_depends_free(fall_depends);
        repo_free(repo,0);
        pool_free(pool);
        return false;
    }

    filename fname = get_filename(
        (Packages){(char **)unique.found, unique.size_found},
        &pool
    );


    for(int i = 0; i < fname.size_locations; i++)
    {
        fprintf(
            stdout,
            " %s",
            fname.filenames[i]
        );
    }
    fprintf(
        stdout,
        "\n"
    );


    for(int i = 0; i < fname.size_notfound_pkgs; i++)
    {
        fprintf(
            stderr,
            " [%s]",
            fname.notfound_pkgs[i]
        );
    }
    fprintf(
        stderr,
        "\n"
    );


    get_filename_free(fname);
    unique_depends_free(unique);
    found_depends_free(fall_depends);

    repo_free(repo, 0);
    pool_free(pool);

    return true;
}

bool show_all_depends__show_location (
    Packages packages,
    const char* name_file
)
{
    Pool *pool;
    Repo *repo;

    if(!load_solvpackages(
        &pool,
        &repo,
        name_file
    ))
        return false;

    found_depends fall_depends = get_all_depends(
        packages,
        &pool
    );

    if (fall_depends.error)
    {
        found_depends_free(fall_depends);
        repo_free(repo,0);
        pool_free(pool);
        return false;
    }

    unique_depends unique =
        get_unique_depends(fall_depends);


    if (unique.error)
    {
        found_depends_free(fall_depends);
        repo_free(repo,0);
        pool_free(pool);
        return false;
    }


    filename fname = get_filename(
        (Packages){(char **)unique.found, unique.size_found},
        &pool
    );


    for(int i = 0; i < fname.size_locations; i++)
    {
        fprintf(
            stdout,
            " %s",
            fname.locations[i]
        );
    }
    fprintf(
        stdout,
        "\n"
    );


    for(int i = 0; i < fname.size_notfound_pkgs; i++)
    {
        fprintf(
            stderr,
            " [%s]",
            fname.notfound_pkgs[i]
        );
    }
    fprintf(
        stderr,
        "\n"
    );


    get_filename_free(fname);
    unique_depends_free(unique);
    found_depends_free(fall_depends);

    repo_free(repo, 0);
    pool_free(pool);

    return true;
}
