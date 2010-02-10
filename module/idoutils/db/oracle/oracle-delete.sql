declare
v_new_tab_name varchar2(35);
begin
    for c1 in (select table_name from user_tables) loop
          v_new_tab_name := '' || C1.Table_Name|| '' ;
          execute immediate ('delete from '||v_new_tab_name) ;
          commit;
    end loop;
end;
/
