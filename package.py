from spack import *

class Funhpc(CMakePackage):
    """FunHPC: Functional HPC Programming"""
    homepage = "https://github.com/eschnett/FunHPC.cxx"
    url= "https://github.com/eschnett/FunHPC.cxx/archive/version/0.1.0.tar.gz"

    version('1.3.1', '935333f47790e2bc8558b43bd6a2033b')
    version('1.3.0', '71a1e57c4d882cdf001f29122edf7fc6')
    version('1.2.0', 'ba2bbeea3091e999b6b85eaeb1b67a83')
    version('1.1.1', '7b9ef638b02fffe35b75517e8eeff580')
    version('1.1.0', '897bd968c42cd4f14f86fcf67da70444')
    version('1.0.0', 'f34e71ccd5548b42672e692c913ba5ee')
    version('0.1.1', 'f0248710f2de88ed2a595ad40d99997c')
    version('0.1.0', '00f7dabc08ed1ab77858785ce0809f50')
    version('master',
            git='https://github.com/eschnett/FunHPC.cxx', branch='master')

    variant('pic', default=True,
            description="Produce position-independent code")

    depends_on('cereal')
    depends_on('googletest')
    # depends_on('hwloc')
    depends_on('jemalloc')
    # depends_on('mpi')
    depends_on('qthreads')

    def cmake_args(self):
        spec = self.spec
        options = []
        if '+pic' in spec:
            options.extend(["-DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=true"])
        return options

    def check(self):
        with working_dir(self.build_directory):
            make("test", "CTEST_OUTPUT_ON_FAILURE=1")
